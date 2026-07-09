/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ArenaWidgetFactory.h"
#include "HubFrame.h"
#include "ChatSearchBar.h"
#include "HubFrameMenu.h"
#include "PMWindow.h"
#include "WulforUtil.h"
#include "AppTheme.h"
#include "Antispam.h"
#include "HubManager.h"
#include "Notification.h"
#include "ShellCommandRunner.h"
#include "EmoticonDialog.h"
#include "WulforSettings.h"
#include "FlowLayout.h"
#include "SearchFrame.h"
#include "Secretary.h"
#ifdef USE_ASPELL
#include "SpellCheck.h"
#endif
#include "ArenaWidgetManager.h"
#include "MainWindow.h"
#include "GlobalTimer.h"

#include "UserListModel.h"
#include "EmoticonFactory.h"

#include "dcpp/LogManager.h"
#include "dcpp/User.h"
#include "dcpp/UserCommand.h"
#include "dcpp/CID.h"
#include "dcpp/HashManager.h"
#include "dcpp/Util.h"
#include "dcpp/ChatMessage.h"

#if HAVE_MALLOC_TRIM
#include <malloc.h>
#endif

#include <QMouseEvent>
#include <QTextCodec>
#include <QItemSelectionModel>
#include <QMenu>
#include <QClipboard>
#include <QInputDialog>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextDocument>
#include <QUrl>
#include <QCloseEvent>
#include <QDateTime>
#include <QThread>
#include <QRegExp>
#include <QScrollBar>
#include <QShortcut>
#include <QHeaderView>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

#include <QtDebug>

#include <exception>

#include "HubFramePrivate.h"

static inline void clearLayout(QLayout *l){
    if (!l)
        return;

    QLayoutItem *item = nullptr;
    while ((item = l->takeAt(0))){
        l->removeWidget(item->widget());
        item->widget()->deleteLater();

        delete item;
    }

    l->invalidate();
}



HubFrame::HubFrame(QWidget *parent, QString hub="", QString encoding="")
    : QWidget(parent)
    , d_ptr(new HubFramePrivate())
{
    Q_D(HubFrame);

    d->total_shared = 0;
    d->arenaMenu = nullptr;
    d->codec = nullptr;
    d->chatDisabled = false;
    d->hasMessages = false;
    d->hasHighlightMessages = false;
    d->client = nullptr;

    setupUi(this);

    Menu::newInstance();

    d->client = ClientManager::getInstance()->getClient(hub.toStdString());
    d->client->addListener(this);

    QString enc = WulforUtil::getInstance()->qtEnc2DcEnc(encoding);

    if (enc.isEmpty())
        enc = WulforUtil::getInstance()->qtEnc2DcEnc(WSGET(WS_DEFAULT_LOCALE));

    if (enc.indexOf(" ") > 0){
        enc = enc.left(enc.indexOf(" "));
        enc.replace(" ", "");
    }

    d->client->setEncoding(enc.toStdString());

    d->codec = WulforUtil::getInstance()->codecForEncoding(encoding);

    init();

    FavoriteHubEntry* entry = FavoriteManager::getInstance()->getFavoriteHubEntry(_tq(hub));

    if (entry && entry->getDisableChat())
        disableChat();

    d->client->connect();

    setAttribute(Qt::WA_DeleteOnClose);

    d->out_messages_index = 0;
    d->out_messages_unsent = false;

    FavoriteManager::getInstance()->addListener(this);
}


HubFrame::~HubFrame(){
    Q_D(HubFrame);

    Menu::deleteInstance();

    treeView_USERS->setModel(nullptr);

    delete d->proxy;
    delete d->model;

    delete d;
}

bool HubFrame::eventFilter(QObject *obj, QEvent *e){
    Q_D(HubFrame);

    if (e->type() == QEvent::KeyRelease){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        const bool keyEnter = (k_e->key() == Qt::Key_Enter || k_e->key() == Qt::Key_Return);
        const bool shiftModifier = (k_e->modifiers() == Qt::ShiftModifier);

        if ((static_cast<QTextEdit*>(obj) == plainTextEdit_INPUT) &&
            (keyEnter && shiftModifier))
        {
            return true;
        }
        else if (static_cast<QLineEdit*>(obj) == lineEdit_FIND) {
            const bool ret = QWidget::eventFilter(obj, e);

            if (keyEnter) {
                d->chatSearch->findForward();
            }

            return ret;
        }

    }
    else if (e->type() == QEvent::KeyPress){
        QKeyEvent *k_e = reinterpret_cast<QKeyEvent*>(e);

        const bool controlModifier = (k_e->modifiers() == Qt::ControlModifier);

        if (static_cast<QTextEdit*>(obj) == plainTextEdit_INPUT)
        {
            const bool useCtrlEnter = WBGET(WB_USE_CTRL_ENTER);
            const bool keyEnter = (k_e->key() == Qt::Key_Enter || k_e->key() == Qt::Key_Return);
            const bool shiftModifier = (k_e->modifiers() == Qt::ShiftModifier);

            if ((useCtrlEnter && keyEnter && controlModifier) ||
                (!useCtrlEnter && keyEnter && !controlModifier && !shiftModifier))
            {
                sendChat(plainTextEdit_INPUT->toPlainText(), false, false);

                plainTextEdit_INPUT->setPlainText("");

                return true;
            }
        }

        if (qobject_cast<LineEdit*>(obj) == lineEdit_FIND && k_e->key() == Qt::Key_Escape){
            lineEdit_FIND->clear();
            slotHideSearchBar();
            return true;
        }

        if (controlModifier) {
            if (k_e->key() == Qt::Key_Equal || k_e->key() == Qt::Key_Plus){
                textEdit_CHAT->zoomIn();

                return true;
            }
            else if (k_e->key() == Qt::Key_Minus){
                textEdit_CHAT->zoomOut();

                return true;
            }
        }
    }
    else if (e->type() == QEvent::MouseButtonPress){
        QMouseEvent *m_e = reinterpret_cast<QMouseEvent*>(e);

        const bool isChat = (static_cast<QWidget*>(obj) == textEdit_CHAT->viewport());
        const bool isUserList = (static_cast<QWidget*>(obj) == treeView_USERS->viewport());

        if (isChat)
            textEdit_CHAT->setExtraSelections(QList<QTextEdit::ExtraSelection>());

        if (isChat && (m_e->button() == Qt::LeftButton)){
            QString pressedParagraph = textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos()));

            if (!WulforUtil::getInstance()->openUrl(pressedParagraph)){
                /**
                  Do nothing
                */
            }
        }
        else if ((isChat || isUserList) && m_e->button() == Qt::MidButton)
        {
            QString nick;
            QString cid;
            bool cursoratnick = false;

            if (isChat){
                QTextCursor cursor = textEdit_CHAT->textCursor();
                const QString pressedParagraph = cursor.block().text();
                const int positionCursor = cursor.columnNumber();

                int l = pressedParagraph.indexOf(" <");
                int r = pressedParagraph.indexOf("> ");

                if (l < r){
                    nick = pressedParagraph.mid(l+2, r-l-2);
                    cid = d->model->CIDforNick(nick, _q(d->client->getHubUrl()));
                }
                if ((positionCursor < r) && (positionCursor > l))
                    cursoratnick = true;
            }
            else if (isUserList){
                QModelIndex index = treeView_USERS->indexAt(treeView_USERS->viewport()->mapFromGlobal(QCursor::pos()));

                if (d->proxy && treeView_USERS->model() == d->proxy)
                    index = d->proxy->mapToSource(index);

                if (index.isValid()){
                    UserListItem *i = reinterpret_cast<UserListItem*>(index.internalPointer());

                    nick = i->getNick();
                    cid = i->getCID();
                }
            }

            if (!cid.isEmpty()){
                if (WIGET(WI_CHAT_MDLCLICK_ACT) == 0){
                    if(!plainTextEdit_INPUT->textCursor().position())
                        plainTextEdit_INPUT->textCursor().insertText(nick + WSGET(WS_CHAT_SEPARATOR) + " ");
                    else
                        plainTextEdit_INPUT->textCursor().insertText(nick + " ");

                    plainTextEdit_INPUT->setFocus();
                }
                else if (WIGET(WI_CHAT_MDLCLICK_ACT) == 2 && (cursoratnick || isUserList))
                    addPM(cid, "", false);
                else if (cursoratnick || isUserList)
                    browseUserFiles(cid, false);
            }
        }
    }
    else if (e->type() == QEvent::MouseButtonDblClick){
        bool isChat = (static_cast<QWidget*>(obj) == textEdit_CHAT->viewport());
        bool isUserList = (static_cast<QWidget*>(obj) == treeView_USERS->viewport());

        if (isChat || isUserList){
            QString nick = "";
            QString cid = "";
            bool cursoratnick = false;
            QTextCursor cursor = textEdit_CHAT->textCursor();

            if (isChat){
                QString nickstatus="",nickmessage="";
                QString pressedParagraph = cursor.block().text();
                //qDebug() << pressedParagraph;
                int positionCursor = cursor.columnNumber();
                int l = pressedParagraph.indexOf(" <");
                int r = pressedParagraph.indexOf("> ");
                if (l < r)
                    nickmessage = pressedParagraph.mid(l+2, r-l-2);
                else {
                    int l1 = pressedParagraph.indexOf(" * ");
                    //qDebug() << positionCursor << " " << l1 << " " << l << " " << r;
                    if (l1 > -1 ) {
                        QString pressedParagraphstatus = pressedParagraph.remove(0,l1+3).simplified();
                        //qDebug() << pressedParagraphstatus;
                        int r1 = pressedParagraphstatus.indexOf(" ");
                        //qDebug() << r1;
                        nickstatus = pressedParagraphstatus.mid(0, r1);
                        //qDebug() << nickstatus;
                    }
                }
                if (!nickmessage.isEmpty() || !nickstatus.isEmpty()) {
                    //qDebug() << nickstatus;
                    //qDebug() << nickmessage;
                    nick = nickmessage + nickstatus;
                    //qDebug() << nick;
                    cid = d->model->CIDforNick(nick, _q(d->client->getHubUrl()));
                    //qDebug() << cid;
                    }
                if (((positionCursor < r) && (positionCursor > l))/* || positionCursor > l1*/)
                    cursoratnick = true;
            }
            else if (isUserList){
                QModelIndex index = treeView_USERS->indexAt(treeView_USERS->viewport()->mapFromGlobal(QCursor::pos()));

                if (d->proxy && treeView_USERS->model() == d->proxy)
                    index = d->proxy->mapToSource(index);

                if (index.isValid()){
                    UserListItem *i = reinterpret_cast<UserListItem*>(index.internalPointer());

                    nick = i->getNick();
                    cid = i->getCID();
                }
            }

            if (!cid.isEmpty()){
                if (WIGET(WI_CHAT_DBLCLICK_ACT) == 1 && (cursoratnick || isUserList)){
                    browseUserFiles(cid, false);
                }
                else if (WIGET(WI_CHAT_DBLCLICK_ACT) == 2 && (cursoratnick || isUserList)){
                    addPM(cid, "", false);
                }
                else if (textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos())).startsWith("user://") || isUserList){//may be dbl click on user nick
                    if(!plainTextEdit_INPUT->textCursor().position())
                        plainTextEdit_INPUT->textCursor().insertText(nick + WSGET(WS_CHAT_SEPARATOR) + " ");
                    else
                        plainTextEdit_INPUT->textCursor().insertText(nick + " ");

                    plainTextEdit_INPUT->setFocus();
                }
            }
        }
    }
    else if (e->type() == QEvent::MouseMove && (static_cast<QWidget*>(obj) == textEdit_CHAT->viewport())){
        if (!textEdit_CHAT->anchorAt(textEdit_CHAT->mapFromGlobal(QCursor::pos())).isEmpty())
            textEdit_CHAT->viewport()->setCursor(Qt::PointingHandCursor);
        else
            textEdit_CHAT->viewport()->setCursor(Qt::IBeamCursor);
    }

    return QWidget::eventFilter(obj, e);
}

void HubFrame::closeEvent(QCloseEvent *e){
    Q_D(HubFrame);

    if (!d->client)
        return;

    blockSignals(true);

    QObject::disconnect(this, nullptr, this, nullptr);

    FavoriteManager::getInstance()->removeListener(this);

    HubManager::getInstance()->unregisterHubUrl(_q(d->client->getHubUrl()));

    d->client->removeListener(this);
    d->client->disconnect(true);
    ClientManager::getInstance()->putClient(d->client);
    d->client = nullptr;

    save();

    for (const auto &it : d->pm){
        PMWindow *w = it;

        disconnect(w, SIGNAL(privateMessageClosed(QString)), this, SLOT(slotPMClosed(QString)));

        if (!isUnload())
            ArenaWidgetManager::getInstance()->rem(w);
    }

    d->pm.clear();

    for (const auto &r : d->shell_list){
        r->cancel();
        r->exit(0);

        r->wait(100);

        if (r->isRunning())
            r->terminate();

        delete r;
    }

    if (isVisible())
        HubManager::getInstance()->setActiveHub(nullptr);

    setAttribute(Qt::WA_DeleteOnClose);

    e->accept();

    blockSignals(false);
    if (!isUnload()) {
        emit closeRequest();
    }
    blockSignals(true);
}

void HubFrame::showEvent(QShowEvent *e){
    Q_D(HubFrame);

    e->accept();

    d->drawLine = false;

    HubManager::getInstance()->setActiveHub(this);

    d->hasMessages = false;
    d->hasHighlightMessages = false;

    MainWindow::getInstance()->redrawToolPanel();
}

void HubFrame::hideEvent(QHideEvent *e){
    Q_D(HubFrame);

    e->accept();

    d->drawLine = true;

    if (!isVisible())
        HubManager::getInstance()->setActiveHub(nullptr);
}

void HubFrame::init(){
    Q_D(HubFrame);

    d->model = new UserListModel(this);
    d->proxy = nullptr;

    splitter_2->setHandleWidth(4);
    splitter_2->setChildrenCollapsible(false);

    treeView_USERS->setModel(d->model);
    treeView_USERS->setSortingEnabled(true);
    treeView_USERS->setItemsExpandable(false);
    treeView_USERS->setUniformRowHeights(true);
    treeView_USERS->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_USERS->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_USERS->header()->hideSection(COLUMN_EXACT_SHARE);
    treeView_USERS->viewport()->installEventFilter(this);

    installEventFilter(this);
    lineEdit_FILTER->installEventFilter(this);
    lineEdit_FIND->installEventFilter(this);

    textEdit_CHAT->document()->setMaximumBlockCount(WIGET(WI_CHAT_MAXPARAGRAPHS));
    textEdit_CHAT->setContextMenuPolicy(Qt::CustomContextMenu);
    textEdit_CHAT->setReadOnly(true);
    textEdit_CHAT->setAutoFormatting(QTextEdit::AutoNone);
    textEdit_CHAT->viewport()->installEventFilter(this); // QTextEdit don't receive all mouse events
    textEdit_CHAT->setMouseTracking(true);

    if (WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance())
        EmoticonFactory::getInstance()->addEmoticons(textEdit_CHAT->document());

    d->chatSearch = new ChatSearchBar(searchFrame, lineEdit_FIND, toolButton_BACK, toolButton_FORWARD,
                                       toolButton_ALL, toolButton_HIDE, textEdit_CHAT, this);

    for (int i = 0; i < d->model->columnCount(); i++)
        comboBox_COLUMNS->addItem(d->model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());

    toolButton_SMILE->setVisible(WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance());
    toolButton_SMILE->setContextMenuPolicy(Qt::CustomContextMenu);
    toolButton_SMILE->setIcon(WICON(WulforUtil::eiEMOTICON));
    AppTheme::applyControlButton(toolButton_SMILE);

    toolButton_HIDE->setIcon(WICON(WulforUtil::eiEDITDELETE));

    frame_SMILES->setLayout(new FlowLayout(frame_SMILES));
    frame_SMILES->setVisible(false);

    QSize sz;
    Q_UNUSED(sz);

    if (EmoticonFactory::getInstance())
        EmoticonFactory::getInstance()->fillLayout(frame_SMILES->layout(), sz);

    for (const auto &l : frame_SMILES->findChildren<EmoticonLabel*>())
        connect(l, SIGNAL(clicked()), this, SLOT(slotSmileClicked()));

    connect(this, SIGNAL(coreConnecting(QString)), this, SLOT(addStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreConnected(QString)), this, SLOT(addStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUserUpdated(dcpp::UserPtr,dcpp::Identity)), this, SLOT(userUpdated(dcpp::UserPtr,dcpp::Identity)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreUserRemoved(dcpp::UserPtr,dcpp::Identity)), this, SLOT(userRemoved(dcpp::UserPtr,dcpp::Identity)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreStatusMsg(QString)), this, SLOT(addStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreFollow(QString)), this, SLOT(follow(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreFailed()), this, SLOT(clearUsers()), Qt::QueuedConnection);
    connect(this, SIGNAL(corePassword()), this, SLOT(getPassword()), Qt::QueuedConnection);
    connect(this, SIGNAL(coreMessage(VarMap)), this, SLOT(newMsg(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(corePrivateMsg(VarMap)), this, SLOT(newPm(VarMap)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreHubUpdated()), MainWindow::getInstance(), SLOT(redrawToolPanel()), Qt::QueuedConnection);
    connect(this, SIGNAL(coreFavoriteUserAdded(QString)), this, SLOT(changeFavStatus(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(coreFavoriteUserRemoved(QString)), this, SLOT(changeFavStatus(QString)), Qt::QueuedConnection);

    connect(label_LAST_STATUS, SIGNAL(linkActivated(QString)), this, SLOT(slotStatusLinkOpen(QString)));
    connect(treeView_USERS, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotUserListMenu(QPoint)));
    connect(treeView_USERS->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderMenu(QPoint)));
    connect(GlobalTimer::getInstance(), SIGNAL(second()), this, SLOT(slotUsersUpdated()));
    connect(textEdit_CHAT, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotChatMenu(QPoint)));
    connect(lineEdit_FILTER, SIGNAL(textChanged(QString)), this, SLOT(slotFilterTextChanged()));
    connect(comboBox_COLUMNS, SIGNAL(activated(int)), this, SLOT(slotFilterTextChanged()));
    connect(toolButton_SMILE, SIGNAL(clicked()), this, SLOT(slotSmile()));
    connect(toolButton_SMILE, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotSmileContextMenu()));
    connect(WulforSettings::getInstance(), SIGNAL(strValueChanged(QString, QString)), this, SLOT(slotSettingsChanged(QString,QString)));
    connect(WulforSettings::getInstance(), SIGNAL(intValueChanged(QString,int)), this, SLOT(slotBoolSettingsChanged(QString,int)));

    verticalLayout_2->setSpacing(4);
    verticalLayout_3->setSpacing(4);
    verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    verticalLayout_3->setContentsMargins(0, 0, 0, 0);

    horizontalLayout_2->setSpacing(8);
    horizontalLayout_2->setAlignment(Qt::AlignVCenter);
    horizontalLayout_2->setStretch(0, 1);

    gridLayout->setHorizontalSpacing(8);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    comboBox_COLUMNS->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    comboBox_COLUMNS->setMinimumContentsLength(8);

#ifdef USE_ASPELL
    connect(plainTextEdit_INPUT, SIGNAL(textChanged()), this, SLOT(slotInputTextChanged()));
    connect(plainTextEdit_INPUT, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotInputContextMenu()));

    plainTextEdit_INPUT->setContextMenuPolicy(Qt::CustomContextMenu);
#endif

    plainTextEdit_INPUT->setWordWrapMode(QTextOption::NoWrap);
    plainTextEdit_INPUT->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    plainTextEdit_INPUT->installEventFilter(this);
    plainTextEdit_INPUT->setAcceptRichText(false);

    textEdit_CHAT->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit_CHAT->setTabStopDistance(40);
    updateStyles();

    load();

    d->completer = new QCompleter(this);
    d->completer->setMaxVisibleItems(10); // This property was introduced in Qt 4.6.
    plainTextEdit_INPUT->setCompleter(d->completer, d->model);

    syncFieldHeights();

    slotSettingsChanged(WS_APP_EMOTICON_THEME, WSGET(WS_APP_EMOTICON_THEME));//toggle emoticon button
}

void HubFrame::initMenu(){
    Q_D(HubFrame);
    WulforUtil *WU = WulforUtil::getInstance();

    delete d->arenaMenu;

    d->arenaMenu = new QMenu(tr("Hub menu"), this);

    QAction *reconnect = new QAction(WU->getPixmap(WulforUtil::eiRECONNECT), tr("Reconnect"), d->arenaMenu);
    QAction *show_wnd  = new QAction(WU->getPixmap(WulforUtil::eiCHAT), tr("Show widget"), d->arenaMenu);
    QAction *addToFav  = new QAction(WU->getPixmap(WulforUtil::eiFAVSERVER), tr("Add to Favorites"), d->arenaMenu);
    QMenu   *copyInfo  = new QMenu(tr("Copy"), d->arenaMenu);
    QAction *copyIP    = copyInfo->addAction(tr("Hub IP"));
    QAction *copyURL   = copyInfo->addAction(tr("Hub URL"));
    QAction *copyTitle = copyInfo->addAction(tr("Hub Title"));

    QAction *sep       = new QAction(d->arenaMenu);
    sep->setSeparator(true);
    QAction *close_wnd = new QAction(WU->getPixmap(WulforUtil::eiEXIT), tr("Close"), d->arenaMenu);

    d->arenaMenu->addActions(QList<QAction*>() << reconnect
                                            << show_wnd
                                            << addToFav
                         );

    d->arenaMenu->addMenu(copyInfo);

    if (d->client && d->client->isConnected()){
        QMenu *u_c = WulforUtil::getInstance()->buildUserCmdMenu(d->client->getHubUrl(), UserCommand::CONTEXT_HUB, d->arenaMenu);

        if (u_c){
            if (!u_c->actions().isEmpty()){
                u_c->setTitle(tr("Hub Menu"));

                d->arenaMenu->addMenu(u_c);

                connect(u_c, SIGNAL(triggered(QAction*)), this, SLOT(slotHubMenu(QAction*)));
            }
        }
    }

    d->arenaMenu->addActions(QList<QAction*>() << sep << close_wnd);

    connect(reconnect,  SIGNAL(triggered()), this, SLOT(slotReconnect()));
    connect(show_wnd,   SIGNAL(triggered()), this, SLOT(slotShowWnd()));
    connect(addToFav,   SIGNAL(triggered()), this, SLOT(addAsFavorite()));
    connect(copyIP,     SIGNAL(triggered()), this, SLOT(slotCopyHubIP()));
    connect(copyTitle,  SIGNAL(triggered()), this, SLOT(slotCopyHubTitle()));
    connect(copyURL,    SIGNAL(triggered()), this, SLOT(slotCopyHubURL()));
    connect(close_wnd,  SIGNAL(triggered()), this, SLOT(slotClose()));
}


void HubFrame::save(){
    Q_D(HubFrame);

    WSSET(WS_CHAT_USERLIST_STATE, treeView_USERS->header()->saveState().toBase64());
    WISET(WI_CHAT_WIDTH, textEdit_CHAT->width());
    WISET(WI_CHAT_USERLIST_WIDTH, treeView_USERS->width());
    WISET(WI_CHAT_SORT_COLUMN, d->model->getSortColumn());
    WISET(WI_CHAT_SORT_ORDER, WulforUtil::getInstance()->sortOrderToInt(d->model->getSortOrder()));
    WSSET("hubframe/chat-background-color", textEdit_CHAT->palette().color(QPalette::Active, QPalette::Base).name());
}

void HubFrame::load(){
    const int w_chat = WIGET(WI_CHAT_WIDTH), w_ulist = WIGET(WI_CHAT_USERLIST_WIDTH);

    QString ustate = WSGET(WS_CHAT_USERLIST_STATE);

    WulforUtil::restoreTreeHeader(treeView_USERS->header(), QByteArray::fromBase64(ustate.toUtf8()));

    if (w_chat >= 0 && w_ulist >= 0){
        QList<int> frames;

        frames << w_chat << w_ulist;

        splitter_2->setSizes(frames);
    }

    treeView_USERS->sortByColumn(WIGET(WI_CHAT_SORT_COLUMN), WulforUtil::getInstance()->intToSortOrder(WIGET(WI_CHAT_SORT_ORDER)));

    reloadSomeSettings();
}

void HubFrame::reloadSomeSettings(){
    label_USERSTATE->setVisible(WBGET(WB_USERS_STATISTICS));

    label_LAST_STATUS->setVisible(WBGET(WB_LAST_STATUS));

    QPalette p = textEdit_CHAT->palette();
    QColor clr = AppTheme::chatBackground();

    if (WBGET("hubframe/change-chat-background-color", false)){
        clr.setNamedColor(WSGET("hubframe/chat-background-color"));

        if (!clr.isValid() || AppTheme::isLegacyBackground(clr))
            clr = AppTheme::chatBackground();
    }

    p.setColor(QPalette::Base, clr);
    textEdit_CHAT->setPalette(p);
}

QWidget *HubFrame::getWidget(){
    return this;
}

QString HubFrame::getArenaTitle(){
    Q_D(HubFrame);
    QString ret = tr("Not connected");

    if (d->client && d->client->isConnected()){
        ret  = QString("%1 - %2 [%3]").arg(_q(d->client->getHubName()))
                                      .arg(_q(d->client->getHubDescription()))
                                      .arg(_q(d->client->getIp()));
        QString prefix = QString("[+%1] ").arg(d->client->isSecure()? ("S") : (d->client->isTrusted()? ("T"): ("")));

        ret.prepend(prefix);
    }
    else if (d->client){
        ret = QString("[-] %1").arg(_q(d->client->getHubUrl()));
    }

    return ret;
}

QString HubFrame::getCIDforNick(QString nick){
    Q_D(HubFrame);

    return d->model->CIDforNick(nick, _q(d->client->getHubUrl()));
}

QString HubFrame::getArenaShortTitle(){
    Q_D(HubFrame);
    QString ret = tr("Not connected");

    if (d->client && d->client->isConnected()){
        ret = QString("[+] %1").arg(_q(d->client->getHubName()));
    }
    else if (d->client){
        ret = QString("[-] %1").arg(_q(d->client->getHubUrl()));
    }

    return ret;
}

QMenu *HubFrame::getMenu(){
    initMenu();

    Q_D(HubFrame);

    return d->arenaMenu;
}

const QPixmap &HubFrame::getPixmap(){
    Q_D(HubFrame);

    if (d->hasHighlightMessages)
        return WICON(WulforUtil::eiMESSAGE);
    else if (d->hasMessages)
        return WICON(WulforUtil::eiHUBMSG);
    else
        return WICON(WulforUtil::eiSERVER);
}

void HubFrame::clearChat(){
    textEdit_CHAT->setHtml("");
    addStatus(tr("Chat cleared."));

    updateStyles();

    if (WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance())
        EmoticonFactory::getInstance()->addEmoticons(textEdit_CHAT->document());
}

void HubFrame::disableChat(){
    Q_D(HubFrame);

    if (!d->chatDisabled){
        addStatus(tr("Chat disabled."));

        d->chatDisabled = true;
    }
    else{
        d->chatDisabled = false;

        addStatus(tr("Chat enabled."));
    }

    plainTextEdit_INPUT->setEnabled(!d->chatDisabled);
    frame_INPUT->setVisible(!d->chatDisabled);
}

void HubFrame::getStatistic(quint64 &users, quint64 &share) const{
    Q_D(const HubFrame);

    if (d->model)
        users = d->model->rowCount();

    share = d->total_shared;
}

bool HubFrame::isConnected() const {
    Q_D(const HubFrame);

    return (d->client? d->client->isConnected() : false);
}

QString HubFrame::getUserInfo(UserListItem *item){
    Q_D(HubFrame);
    QString ttip = "";

    ttip += d->model->headerData(COLUMN_NICK, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getNick() + "\n";
    ttip += d->model->headerData(COLUMN_COMMENT, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getComment() + "\n";
    ttip += d->model->headerData(COLUMN_EMAIL, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getEmail() + "\n";
    ttip += d->model->headerData(COLUMN_IP, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getIP() + "\n";
    ttip += d->model->headerData(COLUMN_SHARE, Qt::Horizontal, Qt::DisplayRole).toString() + ": " +
            WulforUtil::formatBytes(item->getShare()) + "\n";
    ttip += d->model->headerData(COLUMN_TAG, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getTag() + "\n";
    ttip += d->model->headerData(COLUMN_CONN, Qt::Horizontal, Qt::DisplayRole).toString() + ": " + item->getConnection() + "\n";

    if (item->isOP())
        ttip += tr("Hub role: Operator");
    else
        ttip += tr("Hub role: User");

    if (item->isFav())
        ttip += tr("\nFavorite user");

    return ttip;
}

void HubFrame::sendMsg(const QString &msg){
    sendChat(msg, false, false);
}

void HubFrame::sendChat(QString msg, bool thirdPerson, bool stripNewLines){
    Q_D(HubFrame);

    if (!d->client || !d->client->isConnected() || msg.isEmpty() || msg.isNull())
        return;

    if (stripNewLines)
        msg.replace("\n", "");

    if (msg.trimmed().isEmpty())
        return;

    if (msg.endsWith("\n"))
        msg = msg.left(msg.lastIndexOf("\n"));

    bool script_ret = false;
#ifdef LUA_SCRIPT
    script_ret = ((ClientScriptInstance *) (d->client))->onHubFrameEnter(d->client, msg.toStdString());
#endif
    if (!script_ret && !parseForCmd(msg, this))
        d->client->hubMessage(msg.toStdString(), thirdPerson);

    //qDebug() << "cmd: " << cmd <<" sript_ret: " << script_ret;
    if (!thirdPerson){
        if (d->out_messages_unsent){
            d->out_messages.removeLast();
            d->out_messages_unsent = false;
        }

        d->out_messages << msg;

        if (d->out_messages.size() > WIGET(WI_OUT_IN_HIST))
            d->out_messages.removeFirst();

        d->out_messages_index = d->out_messages.size()-1;
    }
}

bool HubFrame::parseForCmd(QString line, QWidget *wg){
    HubFrame *fr = qobject_cast<HubFrame *>(wg);
    PMWindow *pm = qobject_cast<PMWindow *>(wg);
    Q_D(HubFrame);

    QStringList list = line.split(" ", WULFOR_SKIP_EMPTY);

    if (list.isEmpty())
        return false;

    if (!line.startsWith("/"))
        return false;

    QString cmd = list.at(0);
    QString param = "";
    bool emptyParam = true;

    if (list.size() > 1){
        param = list.at(1);
        emptyParam = false;
    }

    if (cmd == "/away"){
        if (Util::getAway() && emptyParam){
            Util::setAway(false);
            Util::setManualAway(false);

            if (fr == this)
                addStatus(tr("Away mode off"));
            else if (pm)
                pm->addStatus(tr("Away mode off"));
        }
        else {
            Util::setAway(true);
            Util::setManualAway(true);

            if (!emptyParam){
                line.remove(0, 6);
                Util::setAwayMessage(line.toStdString());
            }

            if (fr == this)
                addStatus(tr("Away mode on: ") + _q(Util::getAwayMessage()));
            else if (pm)
                pm->addStatus(tr("Away mode on: ") + _q(Util::getAwayMessage()));
        }
    }
    else if (cmd == "/alias" && !emptyParam){
        QStringList lex = line.split(" ", WULFOR_SKIP_EMPTY);

        if (lex.size() >= 2){
            QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());

            if (lex.at(1) == "list"){
                if (!aliases.isEmpty()){
                    if (fr == this)
                        addStatus("\n"+aliases);
                    else if (pm)
                        pm->addStatus("\n"+aliases);
                }
                else{
                    if (fr == this)
                        addStatus(tr("Aliases not found."));
                    else if (pm)
                        pm->addStatus(tr("Aliases not found."));
                }
            }
            else if (lex.at(1) == "purge" && lex.size() == 3){
                QString alias = lex.at(2);
                QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);

                for (const auto &line : alias_list){
                    QStringList cmds = line.split('\t', WULFOR_SKIP_EMPTY);

                    if (cmds.size() == 2 && alias == cmds.at(0)){
                        alias_list.removeAt(alias_list.indexOf(line));

                        QString new_aliases;
                        for (const auto &line : alias_list)
                            new_aliases += line + "\n";

                        WSSET(WS_CHAT_CMD_ALIASES, new_aliases.toUtf8().toBase64());

                        if (fr == this)
                            addStatus(tr("Alias removed."));
                        else if (pm)
                            pm->addStatus(tr("Alias removed."));
                    }
                }
            }
            else if (lex.size() >= 2){
                QString raw = line;

                raw.remove(0, raw.indexOf(" ")+1);

                if (raw.indexOf("::") <= 0){
                    if (fr == this)
                        addStatus(tr("Invalid alias syntax."));
                    else if (pm)
                        pm->addStatus(tr("Invalid alias syntax."));
                }
                else {
                    QStringList new_cmd = raw.split("::", WULFOR_SKIP_EMPTY);

                    if (new_cmd.size() < 2 || new_cmd.at(1).isEmpty()){
                        if (fr == this)
                            addStatus(tr("Invalid alias syntax."));
                        else if (pm)
                            pm->addStatus(tr("Invalid alias syntax."));
                    }
                    else if (!aliases.contains(new_cmd.at(0)+'\t')){
                        aliases += new_cmd.at(0) + '\t' +  new_cmd.at(1) + '\n';

                        WSSET(WS_CHAT_CMD_ALIASES, aliases.toUtf8().toBase64());

                        if (fr == this)
                            addStatus(tr("Alias %1 => %2 has been added").arg(new_cmd.at(0)).arg(new_cmd.at(1)));
                        else if (pm)
                            pm->addStatus(tr("Alias %1 => %2 has been added").arg(new_cmd.at(0)).arg(new_cmd.at(1)));
                    }
                }
            }
        }
    }
    else if (cmd == "/kword" && !emptyParam){
        if (list.size() < 2 || list.size() > 3)
            return false;

        enum { List=0, Add, Remove };

        int kw_action = List;

        if (list.at(1) == QString("add"))
            kw_action = Add;
        else if (list.at(1) == QString("purge"))
            kw_action = Remove;
        else if (list.at(1) == QString("list"))
            kw_action = List;
        else {
            if (fr == this)
                addStatus(tr("Invalid command syntax."));
            else if (pm)
                pm->addStatus(tr("Invalid command syntax."));

            return false;
        }

        if (kw_action != List && list.size() != 3){
            if (fr == this)
                addStatus(tr("Invalid command syntax."));
            else if (pm)
                pm->addStatus(tr("Invalid command syntax."));

            return false;
        }

        QStringList kwords = WVGET("hubframe/chat-keywords", QStringList()).toStringList();

        switch (kw_action){
        case List:
            {
                QString str = tr("List of keywords:\n");

                for (const auto &s : kwords)
                    str += "\t" + s + "\n";

                if (fr == this)
                    addStatus(str);
                else if (pm)
                    pm->addStatus(str);;

                break;
            }
        case Remove:
            {
                QString kword = list.last();

                if (kwords.contains(kword))
                    kwords.removeOne(kword);

                break;
            }
        case Add:
            {
                QString kword = list.last();

                if (!kwords.contains(kword))
                    kwords.push_back(kword);

                break;
            }
        default:
            break;
        }

        WVSET("hubframe/chat-keywords", kwords);
    }
    else if (cmd == "/ratio"){
        double ratio;
        double up   = static_cast<double>(SETTING(TOTAL_UPLOAD));
        double down = static_cast<double>(SETTING(TOTAL_DOWNLOAD));


        if (down > 0)
            ratio = up / down;
        else
            ratio = 0;

        QString line = tr("ratio: %1 (uploads: %2, downloads: %3)")
                          .arg(QString().setNum(ratio, 'f', 3))
                          .arg(WulforUtil::formatBytes(up))
                          .arg(WulforUtil::formatBytes(down));

        if (param.trimmed() == "show"){
            if (fr == this)
                sendChat(line, true, false);
            else if (pm)
                pm->sendMessage(line, true, false);
        } else if (emptyParam){
            if (fr == this)
                addStatus(line);
            else if (pm)
                pm->addStatus(line);
        }
    }
    else if (cmd == "/rebuild") {
        HashManager::getInstance()->rebuild();
    }
    else if (cmd == "/refresh") {
        ShareManager::getInstance()->setDirty();
        ShareManager::getInstance()->refresh(true);
    }
#ifdef USE_ASPELL
    else if (cmd == "/aspell" && !emptyParam){
        WBSET(WB_APP_ENABLE_ASPELL, param.trimmed() == "on");

        if (WBGET(WB_APP_ENABLE_ASPELL) && !SpellCheck::getInstance())
            SpellCheck::newInstance();
        else if (SpellCheck::getInstance())
            SpellCheck::deleteInstance();

        if (fr == this)
            addStatus(tr("Aspell switched %1").arg((WBGET(WB_APP_ENABLE_ASPELL)? tr("on") : tr("off"))) );
        else if (pm)
            pm->addStatus(tr("Aspell switched %1").arg((WBGET(WB_APP_ENABLE_ASPELL)? tr("on") : tr("off"))) );
    }
#endif
    else if (cmd == "/back"){
        Util::setAway(false);

        if (fr == this)
            addStatus(tr("Away mode off"));
        else if (pm)
            pm->addStatus(tr("Away mode off"));
    }
    else if (cmd == "/clear"){
        textEdit_CHAT->setHtml("");

        if (fr == this)
            addStatus(tr("Chat has been cleared"));
        else if (pm)
            pm->addStatus(tr("Chat has been cleared"));

    }
    else if (cmd == "/close"){
        this->close();
    }
    else if (cmd == "/fav"){
        addAsFavorite();
    }
    else if (cmd == "/browse" && !emptyParam){
        browseUserFiles(d->model->CIDforNick(param, _q(d->client->getHubUrl())), false);
    }
    else if (cmd == "/grant" && !emptyParam){
        grantSlot(d->model->CIDforNick(param, _q(d->client->getHubUrl())));
    }
    else if (cmd == "/magnet" && !emptyParam){
        WISET(WI_DEF_MAGNET_ACTION, param.toInt());
    }
    else if (cmd == "/info" && !emptyParam){
        UserListItem *item = d->model->itemForNick(param, _q(d->client->getHubUrl()));

        if (item){
            QString ttip = "\n" + getUserInfo(item);

            if (fr == this)
                addStatus(ttip);
            else if (pm)
                pm->addStatus(ttip);
        }
    }
    else if (cmd == "/me" && !emptyParam){
        if (line.endsWith("\n"))//remove extra \n char
            line = line.left(line.lastIndexOf("\n"));

        // This is temporary. It is need to check ClientManager::privateMessage(...) function
        // in dcpp kernel with version > 0.75. And "if (fr == this)" will not be needed.
        if (fr == this)
            line.remove(0, 4);

        if (fr == this)
            sendChat(line, true, false);
        else if (pm)
            pm->sendMessage(line, true, false);
    }
    else if (cmd == "/pm" && !emptyParam){
        addPM(d->model->CIDforNick(param, _q(d->client->getHubUrl())), "", false, param);
    }
    else if (cmd == "/help" || cmd == "/?" || cmd == "/h"){
        QString out = "\n";
#ifdef USE_ASPELL
        out += tr("/aspell on/off - enable/disable spell checking\n");
#endif
        out += tr("/alias <ALIAS_NAME>::<COMMAND> - make alias /ALIAS_NAME to /COMMAND\n");
        out += tr("/alias purge <ALIAS_NAME> - remove alias\n");
        out += tr("/alias list - list all aliases\n");
        out += tr("/away <message> - set away-mode on/off\n");
        out += tr("/back - set away-mode off\n");
        out += tr("/browse <nick> - browse user files\n");
        out += tr("/clear - clear chat window\n");
        out += tr("/kword add <keyword> - add user-defined keyword which will be highlighted in the chat\n");
        out += tr("/kword purge <keyword> - remove user-defined keyword\n");
        out += tr("/kword list - full list of keywords which will be highlighted in the chat\n");
        out += tr("/magnet - default action with magnet (0-ask, 1-search, 2-download)\n");
        out += tr("/close - close this hub\n");
        out += tr("/fav - add this hub to favorites\n");
        out += tr("/grant <nick> - grant extra slot to user\n");
        out += tr("/help, /?, /h - show this help\n");
        out += tr("/info <nick> - show info about user\n");
        out += tr("/ratio [show] - show ratio [send in chat]\n");
        out += tr("/rebuild - rebuild hash\n");
        out += tr("/refresh - update own file list\n");
        out += tr("/me - say a third person\n");
        out += tr("/pm <nick> - begin private chat with user\n");
        out += tr("/ws param value - set gui option param in value (without value return current value of option)\n");
        out += tr("/dcpps param value - set core option param in value (without value return current value of option)\n");
#ifdef LUA_SCRIPT
        out += tr("/luafile <file> - load Lua file\n");
        out += tr("/lua <chunk> - execute Lua chunk\n");
#endif
        out = out.trimmed();

        if (fr == this)
            addStatus(out);
        else if (pm)
            pm->addStatus(out);
    }
#ifdef LUA_SCRIPT
        else if (cmd == "/lua" && !emptyParam) {
            ScriptManager::getInstance()->EvaluateChunk(Text::fromT(_tq(param)));
        }
        else if( cmd == "/luafile" && !emptyParam) {
            ScriptManager::getInstance()->EvaluateFile(Text::fromT(_tq(param)));
        }
#endif
    else if (cmd == "/sh" && !emptyParam){
        if (line.endsWith("\n"))//remove extra \n char
            line = line.left(line.lastIndexOf("\n"));

        line = line.remove(0, 4);

        ShellCommandRunner *sh = new ShellCommandRunner(line, wg);
        connect(sh, SIGNAL(finished(bool,QString)), this, SLOT(slotShellFinished(bool,QString)));

        d->shell_list.append(sh);

        sh->start();
    }
    else if (cmd == "/ws" && !emptyParam){
        line = line.remove(0, 4);
        line.replace("\n", "");
        QString res;
        WSCMD(line,res);
        if (fr == this)
            addStatus(res);
        else if (pm)
            pm->addStatus(res);
    }
    else if (cmd == "/dcpps" && !emptyParam) {
        line = line.remove(0,7);
        QString out = _q(SettingsManager::getInstance()->parseCoreCmd (_tq(line)));
        if (fr == this)
            addStatus(out);
        else if (pm)
            pm->addStatus(out);
    }
    else if (!WSGET(WS_CHAT_CMD_ALIASES).isEmpty()){
        QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());
        QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);
        bool ok = false;

        for (const auto &line : alias_list){
            QStringList cmds = line.split('\t', WULFOR_SKIP_EMPTY);

            if (cmds.size() == 2 && cmd == ("/" + cmds.at(0))){
                parseForCmd(cmds.at(1), wg);

                ok = true;
            }
        }

        if (!ok)
            return ok;
    }
    else
        return false;

    return true;
}

QString HubFrame::getHubUrl() {
    Q_D(HubFrame);

    if (d->client)
        return _q(d->client->getHubUrl());

    return "";
}

QString HubFrame::getHubName() {
    Q_D(HubFrame);

    if (d->client)
        return _q(d->client->getHubName());

    return "";
}

QString HubFrame::getMyNick() {
    Q_D(HubFrame);

    if (d->client)
        return _q(d->client->getMyNick());

    return "";
}

void HubFrame::addStatus(QString msg){
    Q_D(HubFrame);

    if (d->chatDisabled)
        return;

    const QString orig_msg = msg;

    QString pure_msg;
    QString short_msg;
    QString status;

    QString nick = " * ";

    QStringList lines = msg.split(QRegExp("[\\n\\r\\f]+"), WULFOR_SKIP_EMPTY);
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).contains(QRegExp("\\w+"))) {
            short_msg = lines.at(i);
            break;
        }
    }
    if (short_msg.isEmpty() && !lines.isEmpty())
        short_msg = lines.first();

    pure_msg  = LinkParser::parseForLinks(msg.left(WIGET(WI_CHAT_STATUS_MSG_MAX_LEN)), false);
    short_msg = LinkParser::parseForLinks(short_msg, false);
    msg       = LinkParser::parseForLinks(msg, true);

    pure_msg        = "<font color=\"" + AppTheme::chatColor(WS_CHAT_STAT_COLOR) + "\">" + pure_msg + "</font>";
    short_msg       = "<font color=\"" + AppTheme::chatColor(WS_CHAT_STAT_COLOR) + "\">" + short_msg + "</font>";
    msg             = "<font color=\"" + AppTheme::chatColor(WS_CHAT_STAT_COLOR) + "\">" + msg + "</font>";
    QString time    = "";

    if (!WSGET(WS_CHAT_TIMESTAMP).isEmpty())
        time = "<font color=\"" + AppTheme::chatColor(WS_CHAT_TIME_COLOR)+ "\">[" + QDateTime::currentDateTime().toString(WSGET(WS_CHAT_TIMESTAMP)) + "]</font>";

    status   = time + "<font color=\"" + AppTheme::chatColor(WS_CHAT_STAT_COLOR) + "\"><b>" + nick + "</b> </font>";

    QRegExp rot_msg = QRegExp("is(\\s+)kicking(\\s+)(\\S+)*(\\s+)because:");

    bool isRotating = (msg.indexOf("is kicking because:") >= 0) || (rot_msg.indexIn(msg) >= 0);

    if (!(isRotating && WBGET(WB_CHAT_ROTATING_MSGS)))
        addOutput(status + msg);

    if (Secretary::getInstance())
        Secretary::getInstance()->coreStatusMsg("", status + msg, orig_msg, _q(d->client->getIp()));

    label_LAST_STATUS->setText(status + short_msg);

    status += pure_msg;
    WulforUtil::getInstance()->textToHtml(status, false);

    d->status_msg_history.push_back(status);

    if (WIGET(WI_CHAT_STATUS_HISTORY_SZ) > 0){
        while (d->status_msg_history.size() > WIGET(WI_CHAT_STATUS_HISTORY_SZ))
            d->status_msg_history.removeFirst();
    }
    else
        d->status_msg_history.clear();

    label_LAST_STATUS->setToolTip(d->status_msg_history.join("<br/>"));
}

void HubFrame::addOutput(QString msg){
    msg.replace("\r", "");
    msg = "<pre>" + msg + "</pre>";
    textEdit_CHAT->append(msg);
}

void HubFrame::addUserData(const QString &nick){
    QTextDocument *chatDoc = textEdit_CHAT->document();
    for (QTextBlock itu = chatDoc->lastBlock(); itu.isValid(); itu = itu.previous()){
        if (!itu.userData())
            itu.setUserData(new UserListUserData(nick));
        else
            break;
    }
}

void HubFrame::addPM(QString cid, QString output, bool keepfocus, QString nick, bool markUnread){
    Q_D(HubFrame);
    bool redirectToMainChat = WBGET("hubframe/redirect-pm-to-main-chat", true);

    if (!d->pm.contains(cid)){
        PMWindow *p = ArenaWidgetFactory().create<PMWindow, QString, QString>(cid, _q(d->client->getHubUrl()));
        p->textEdit_CHAT->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(p, SIGNAL(privateMessageClosed(QString)), this, SLOT(slotPMClosed(QString)));
        connect(p, SIGNAL(inputTextChanged()), this, SLOT(slotInputTextChanged()));
        connect(p, SIGNAL(inputTextMenu()), this, SLOT(slotInputContextMenu()));
        connect(p->textEdit_CHAT, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotChatMenu(QPoint)));

        p->setCompleter(d->completer, d->model);
        p->setAttribute(Qt::WA_DeleteOnClose);
        p->addOutput(output, markUnread);
        if (!nick.isEmpty())
            p->addUserData(nick);

        if (!(keepfocus && WBGET(WB_CHAT_KEEPFOCUS))){
            ArenaWidgetManager::getInstance()->activate(p);

            p->requestFocus();
        }

        d->pm.insert(cid, p);

        if (!p->isVisible() && redirectToMainChat){
            addOutput("<b>PM: </b>" + output);
            if (!nick.isEmpty())
                addUserData(nick);
        }
    }
    else{
        auto it = d->pm.find(cid);

        if (markUnread && output.indexOf(_q(d->client->getMyNick())) >= 0)
            it.value()->setHasHighlightMessages(true);

        it.value()->addOutput(output, markUnread);
        if (!nick.isEmpty())
            it.value()->addUserData(nick);

        if (!(keepfocus && WBGET(WB_CHAT_KEEPFOCUS))){
            ArenaWidgetManager::getInstance()->activate(it.value());

            it.value()->requestFocus();
        }

        if (! it.value()->isVisible() && redirectToMainChat){
            addOutput("<b>PM: </b>" + output);
            if (!nick.isEmpty())
                addUserData(nick);
        }
    }
}

bool HubFrame::isOP(const QString& nick) {
    Q_D(HubFrame);

    UserListItem *item = d->model->itemForNick(nick, _q(d->client->getHubUrl()));

    return (item? item->isOP() : false);
}



void HubFrame::addAsFavorite(){
    Q_D(HubFrame);
    FavoriteHubEntry *existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(d->client->getHubUrl());

    if (!existingHub){
        FavoriteHubEntry aEntry;

        aEntry.setServer(d->client->getHubUrl());
        aEntry.setName(d->client->getHubName());
        aEntry.setHubDescription(d->client->getHubDescription());
        aEntry.setConnect(false);
        aEntry.setNick(d->client->getMyNick());
        aEntry.setEncoding(d->client->getEncoding());

        FavoriteManager::getInstance()->addFavorite(aEntry);
        FavoriteManager::getInstance()->save();

        addStatus(tr("Favorite hub added."));
    }
    else{
        addStatus(tr("Favorite hub already exists."));
    }
}

void HubFrame::disablePrivateMessages(bool disable) {
    if (disable)
        disconnect(this, SIGNAL(corePrivateMsg(VarMap)), this, SLOT(newPm(VarMap)));
    else
        connect(this, SIGNAL(corePrivateMsg(VarMap)), this, SLOT(newPm(VarMap)), Qt::QueuedConnection);
}

void HubFrame::createPMWindow(const QString &nick){
    Q_D(HubFrame);
    createPMWindow(CID(_tq(d->model->CIDforNick(nick, _q(d->client->getHubUrl())))));
}

void HubFrame::createPMWindow(const dcpp::CID &cid){
    addPM(_q(cid.toBase32()), "");
}

bool HubFrame::hasCID(const dcpp::CID &cid, const QString &nick){
    Q_D(HubFrame);
    return (d->model->CIDforNick(nick, _q(d->client->getHubUrl())) == _q(cid.toBase32()));
}

void HubFrame::clearUsers(){
    Q_D(HubFrame);

    if (d->model){
        d->model->blockSignals(true);
        d->model->clear();
        d-> model->blockSignals(false);
        treeView_USERS->setModel(d->model);
    }

    d->total_shared = 0;

    treeView_USERS->repaint();

    slotUsersUpdated();

    d->model->repaint();
}

void HubFrame::pmUserOffline(const QString &cid){
    pmUserEvent(cid, tr("User offline."));
}

void HubFrame::pmUserEvent(const QString &cid, const QString &e){
    Q_D(HubFrame);

    if (!d->pm.contains(cid))
        return;

    QString output = "";
    QString nick    = " * ";

    QString msg     = "<font color=\"" + AppTheme::chatColor(WS_CHAT_MSG_COLOR) + "\">" + e + "</font>";
    QString time    = "";

    if (!WSGET(WS_CHAT_TIMESTAMP).isEmpty())
        time = "<font color=\""+AppTheme::chatColor(WS_CHAT_TIME_COLOR)+">["+QDateTime::currentDateTime().toString(WSGET(WS_CHAT_TIMESTAMP))+"]</font>";

    output = time + "<font color=\"" + AppTheme::chatColor(WS_CHAT_STAT_COLOR) + "\"><b>" + nick + "</b> </font>";
    output += msg;

    WulforUtil::getInstance()->textToHtml(output, false);

    d->pm[cid]->addOutput(output);
}

void HubFrame::getPassword(){
    Q_D(HubFrame);

    MainWindow *MW = MainWindow::getInstance();

    if (!MW->isVisible() && d->client->getPassword().empty()){
        MW->show();
        MW->raise();

    }

    if(d->client && !d->client->getPassword().empty()) {
        d->client->password(d->client->getPassword());
        addStatus(tr("Stored password sent..."));
    }
    else if (d->client && d->client->isConnected()){
        QString pass = QInputDialog::getText(this, _q(d->client->getHubUrl()), tr("Password"), QLineEdit::Password);

        if (!pass.isEmpty()){
            d->client->setPassword(pass.toStdString());
            d->client->password(pass.toStdString());
        }
        else
            d->client->disconnect(true);
    }
}

void HubFrame::follow(QString redirect){
    if(!redirect.isEmpty()) {
        if(ClientManager::getInstance()->isConnected(_tq(redirect))) {
            addStatus(tr("The hub is trying to redirect you to a hub you are already connected to. Disconnecting."));
            return;
        }

        string url = _tq(redirect);

        Q_D(HubFrame);
        // the client is dead, long live the client!
        d->client->removeListener(this);
        HubManager::getInstance()->unregisterHubUrl(_q(d->client->getHubUrl()));
        ClientManager::getInstance()->putClient(d->client);
        clearUsers();
        d->client = ClientManager::getInstance()->getClient(url);

        d->client->addListener(this);
        d->client->connect();
    }
}

void HubFrame::syncFieldHeights(){
    const int h = qMax(lineEdit_FILTER->sizeHint().height(),
                       comboBox_COLUMNS->sizeHint().height());

    lineEdit_FILTER->setFixedHeight(h);
    comboBox_COLUMNS->setFixedHeight(h);
    plainTextEdit_INPUT->setMinimumHeight(h);
    toolButton_SMILE->setFixedSize(h, h);
}

void HubFrame::updateStyles(){
    QString custom_font_desc = WSGET(WS_CHAT_FONT);
    QFont custom_font;

    if (!custom_font_desc.isEmpty() && custom_font.fromString(custom_font_desc)){
        textEdit_CHAT->document()->setDefaultStyleSheet(
                QString("pre { margin:0px; white-space:pre-wrap; font-family:'%1'; font-size: %2pt; }")
                                                        .arg(custom_font.family()).arg(custom_font.pointSize())
                                                       );
    }
    else {
        textEdit_CHAT->document()->setDefaultStyleSheet(
                                                        QString("pre { margin:0px; white-space:pre-wrap; font-family:'%1' }")
                                                        .arg(QApplication::font().family())
                                                       );
    }

    custom_font_desc = WSGET(WS_CHAT_ULIST_FONT);

    if (!custom_font_desc.isEmpty() && custom_font.fromString(custom_font_desc))
        treeView_USERS->setFont(custom_font);

    syncFieldHeights();
}

void HubFrame::slotActivate(){
    plainTextEdit_INPUT->setFocus();
}

void HubFrame::slotUsersUpdated(){
    Q_D(HubFrame);

    if (d->proxy && treeView_USERS->model() == d->proxy){
        label_USERSTATE->setText(QString(tr("Users count: %3/%1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared))
                                 .arg(d->proxy->rowCount()));
    }
    else {
        label_USERSTATE->setText(QString(tr("Users count: %1 | Total share: %2"))
                                 .arg(d->model->rowCount())
                                 .arg(WulforUtil::formatBytes(d->total_shared)));
    }

    label_LAST_STATUS->setMaximumHeight(label_USERSTATE->height());
}

void HubFrame::slotReconnect(){
    clearUsers();

    Q_D(HubFrame);

    if (d->client)
        d->client->reconnect();
}

void HubFrame::slotMapOnArena(){
    ArenaWidgetManager::getInstance()->activate(this);
}

void HubFrame::slotClose(){
    ArenaWidgetManager::getInstance()->rem(this);
}

void HubFrame::slotPMClosed(QString cid){
    Q_D(HubFrame);

    auto it = d->pm.find(cid);

    if (it != d->pm.end())
        d->pm.erase(it);
}

template < QString (UserListItem::*func)() const >
static void copyTagToClipboard(QModelIndexList &list){
    QString ret = "";
    UserListItem *item = nullptr;

    for (const auto &i : list) {
        item = reinterpret_cast<UserListItem*> ( i.internalPointer() );

        if ( !ret.isEmpty() )
            ret += "\n";

        if ( item )
            ret += (item->*func)();
    }

    qApp->clipboard()->setText ( ret, QClipboard::Clipboard );
}

template < qulonglong (UserListItem::*func)() const >
static void copyTagToClipboard(QModelIndexList &list){
    QString ret = "";
    UserListItem *item = nullptr;

    for (const auto &i : list) {
        item = reinterpret_cast<UserListItem*> ( i.internalPointer() );

        if ( !ret.isEmpty() )
            ret += "\n";

        if ( item )
            ret += WulforUtil::formatBytes((item->*func)());
    }

    qApp->clipboard()->setText ( ret, QClipboard::Clipboard );
}

void HubFrame::slotUserListMenu(const QPoint&){
    QItemSelectionModel *selection_model = treeView_USERS->selectionModel();
    QModelIndexList proxy_list = selection_model->selectedRows(0);

    if (proxy_list.size() < 1)
        return;

    QString cid = "";

    Q_D(HubFrame);

    if (d->proxy && treeView_USERS->model() == d->proxy){
        QModelIndex i = d->proxy->mapToSource(proxy_list.at(0));
        cid = reinterpret_cast<UserListItem*>(i.internalPointer())->getCID();
    }
    else{
        QModelIndex i = proxy_list.at(0);
        cid = reinterpret_cast<UserListItem*>(i.internalPointer())->getCID();
    }

    Menu::Action action = Menu::getInstance()->execUserMenu(d->client, cid);
    UserListItem *item = nullptr;

    proxy_list = selection_model->selectedRows(0);

    if (proxy_list.size() < 1)
        return;

    QModelIndexList list;

    if (d->proxy && treeView_USERS->model() == d->proxy){
        for (const auto &i : proxy_list)
            list.push_back(d->proxy->mapToSource(i));
    }
    else
        list = proxy_list;

    switch (action){
        case Menu::None:
        {
            return;
        }
        case Menu::BrowseFilelist:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    browseUserFiles(item->getCID());
            }

            break;
        }
        case Menu::PrivateMessage:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    addPM(item->getCID(), "", false);
            }

            break;
        }
        case Menu::CopyText:
        {
            QString ttip = "";

            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    ttip += getUserInfo(item) + "\n";

                ttip += "\n";
            }

            if (!ttip.isEmpty())
                qApp->clipboard()->setText(ttip, QClipboard::Clipboard);

            break;
        }
        case Menu::CopyNick:
        {
            copyTagToClipboard<&UserListItem::getNick> (list);

            break;
        }
        case Menu::CopyComment:
        {
            copyTagToClipboard<&UserListItem::getComment> (list);

            break;
        }
        case Menu::CopyIP:
        {
            copyTagToClipboard<&UserListItem::getIP> (list);

            break;
        }
        case Menu::CopyShare:
        {
            copyTagToClipboard<&UserListItem::getShare> (list);

            break;
        }
        case Menu::CopyTag:
        {
            copyTagToClipboard<&UserListItem::getTag> (list);

            break;
        }
        case Menu::CopyEmail:
        {
            copyTagToClipboard<&UserListItem::getEmail> (list);

            break;
        }
        case Menu::MatchQueue:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    browseUserFiles(item->getCID(), true);
            }

            break;
        }
        case Menu::FavoriteAdd:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    addUserToFav(item->getCID());
            }

            break;
        }
        case Menu::FavoriteRem:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    delUserFromFav(item->getCID());
            }

            break;
        }
        case Menu::GrantSlot:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    grantSlot(item->getCID());
            }

            break;
        }
        case Menu::RemoveQueue:
        {
            for (const auto &i : list){
                item = reinterpret_cast<UserListItem*>(i.internalPointer());

                if (item)
                    delUserFromQueue(item->getCID());
            }

            break;
        }
        case Menu::AntiSpamWhite:
        {

            if (AntiSpam::getInstance()){
                for (const auto &i : list){
                    item = reinterpret_cast<UserListItem*>(i.internalPointer());

                    (*AntiSpam::getInstance()) << eIN_WHITE << item->getNick();
                }
            }

            break;
        }
        case Menu::AntiSpamBlack:
        {
            if (AntiSpam::getInstance()){
                for (const auto &i : list){
                    item = reinterpret_cast<UserListItem*>(i.internalPointer());

                    (*AntiSpam::getInstance()) << eIN_BLACK << item->getNick();
                }
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

void HubFrame::slotChatMenu(const QPoint &){
    QTextEdit *editor = qobject_cast<QTextEdit*>(sender());
    if (!editor)
        return;

    QTextCursor cursor = editor->cursorForPosition(editor->mapFromGlobal(QCursor::pos()));
    QString nick;
    if(cursor.block().userData())
        nick = static_cast<UserListUserData*>(cursor.block().userData())->data;

    Q_D(HubFrame);

    QString cid = d->model->CIDforNick(nick, _q(d->client->getHubUrl()));

    if (cid.isEmpty()){
        QMenu *m = editor->createStandardContextMenu(QCursor::pos());
        m->exec(QCursor::pos());

        delete m;

        return;
    }

    QPoint p = QCursor::pos();

    bool pmw = (editor != this->textEdit_CHAT);

    Menu::Action action = Menu::getInstance()->execChatMenu(d->client, cid, pmw);

    switch (action){
        case Menu::CopyText:
        {
            QString ret = editor->textCursor().selectedText();

            if (ret.isEmpty())
                ret = editor->anchorAt(textEdit_CHAT->mapFromGlobal(p));

            if (ret.startsWith("user://")){
                ret.remove(0, 7);

                ret = ret.trimmed();

                if (ret.startsWith("<") && ret.endsWith(">")){
                    ret.remove(0, 1);//remove <
                    ret = ret.left(ret.lastIndexOf(">"));//remove >
                }
            }

            if (ret.isEmpty())
                ret = editor->textCursor().block().text();

            qApp->clipboard()->setText(ret, QClipboard::Clipboard);

            break;
        }
        case Menu::SearchText:
        {
            QString ret = editor->textCursor().selectedText();

            if (ret.isEmpty())
                ret = editor->anchorAt(textEdit_CHAT->mapFromGlobal(p));

            if (ret.startsWith("user://")){
                ret.remove(0, 7);

                ret = ret.trimmed();

                if (ret.startsWith("<") && ret.endsWith(">")){
                    ret.remove(0, 1);//remove <
                    ret = ret.left(ret.lastIndexOf(">"));//remove >
                }
            } else if (ret.startsWith("magnet:?")) {
                QString name, tth;
                int64_t size;

                WulforUtil::splitMagnet(ret, size, tth, name);
                ret = name;
            }

            if (ret.isEmpty())
                break;

            SearchFrame *sf = ArenaWidgetFactory().create<SearchFrame, QWidget*>(this);
            sf->fastSearch(ret, false);

            break;
        }
        case Menu::CopyNick:
        {
            qApp->clipboard()->setText(nick, QClipboard::Clipboard);

            break;
        }
        case Menu::FindInList:
        {
            UserListItem *item = d->model->itemForNick(nick, _q(d->client->getHubUrl()));

            if (item){
                QModelIndex index = d->model->index(item->row(), 0, QModelIndex());

                treeView_USERS->clearSelection();
                treeView_USERS->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                treeView_USERS->scrollTo(index, QAbstractItemView::PositionAtCenter);
            }

            break;
        }
        case Menu::BrowseFilelist:
        {
            browseUserFiles(cid, false);

            break;
        }
        case Menu::MatchQueue:
        {
            browseUserFiles(cid, true);

            break;
        }
        case Menu::PrivateMessage:
        {
            addPM(cid, "", false);

            if (d->pm.contains(cid))
                ArenaWidgetManager::getInstance()->activate(d->pm[cid]);

            break;
        }
        case Menu::FavoriteAdd:
        {
            addUserToFav(cid);

            break;
        }
        case Menu::FavoriteRem:
        {
            delUserFromFav(cid);

            break;
        }
        case Menu::GrantSlot:
        {
            grantSlot(cid);

            break;
        }
        case Menu::RemoveQueue:
        {
            delUserFromQueue(cid);

            break;
        }
        case Menu::UserCommands:
        {
            //All work already done in Menu::exec*
            break;
        }
        case Menu::ClearChat:
        {
            if (pmw)
                MainWindow::getInstance()->slotChatClear(); // some hack
            else
                clearChat();

            break;
        }
        case Menu::FindInChat:
        {
            slotShowSearchBar();

            break;
        }
        case Menu::DisableChat:
        {
            disableChat();

            break;
        }
        case Menu::SelectAllChat:
        {
            editor->selectAll();

            break;
        }
        case Menu::ZoomInChat:
        {
            editor->zoomIn();

            break;
        }
        case Menu::ZoomOutChat:
        {
            editor->zoomOut();

            break;
        }
        case Menu::AntiSpamWhite:
        {
            if (AntiSpam::getInstance())
                (*AntiSpam::getInstance()) << eIN_WHITE << nick;

            break;
        }
        case Menu::AntiSpamBlack:
        {
            if (AntiSpam::getInstance())
                (*AntiSpam::getInstance()) << eIN_BLACK << nick;

            break;
        }
        case Menu::None:
        {
            return;
        }
        default:
        {
            return;
        }
    }
}

void HubFrame::slotHeaderMenu(const QPoint&){
    WulforUtil::headerMenu(treeView_USERS);
}

void HubFrame::slotShowWnd(){
    if (isVisible())
        return;

   ArenaWidgetManager::getInstance()->activate(this);
}

void HubFrame::slotShellFinished(bool ok, QString output){
    if (ok){
        HubFrame *fr = qobject_cast<HubFrame *>(sender()->parent());
        PMWindow *pm = qobject_cast<PMWindow *>(sender()->parent());

        LinkParser::parseForMagnetAlias(output);

        if (fr == this)
            sendChat(output, false, false);
        else if (pm)
            pm->sendMessage(output, false, false);
    }

    ShellCommandRunner *runner = reinterpret_cast<ShellCommandRunner*>(sender());

    runner->cancel();
    runner->exit(0);
    runner->wait(100);

    if (runner->isRunning())
        runner->terminate();

    Q_D(HubFrame);

    if (d->shell_list.indexOf(runner) >= 0)
        d->shell_list.removeAt(d->shell_list.indexOf(runner));

    delete runner;
}

void HubFrame::nextMsg(){
    if (!plainTextEdit_INPUT->hasFocus())
        return;

    Q_D(HubFrame);

    if (d->out_messages_index < 0 ||
        d->out_messages_index+1 > d->out_messages.size()-1 ||
        d->out_messages.isEmpty())
        return;

    if (d->out_messages.at(d->out_messages_index) != plainTextEdit_INPUT->toPlainText())
        d->out_messages[d->out_messages_index] = plainTextEdit_INPUT->toPlainText();

    if (d->out_messages_index+1 <= d->out_messages.size()-1)
        d->out_messages_index++;

    plainTextEdit_INPUT->setPlainText(d->out_messages.at(d->out_messages_index));

    if (d->out_messages_unsent && d->out_messages_index == d->out_messages.size()-1){
        d->out_messages.removeLast();
        d->out_messages_unsent = false;
        d->out_messages_index = d->out_messages.size()-1;
    }
}

void HubFrame::prevMsg(){
    if (!plainTextEdit_INPUT->hasFocus())
        return;

    Q_D(HubFrame);

    if (d->out_messages_index < 1 ||
        d->out_messages_index-1 > d->out_messages.size()-1 ||
        d->out_messages.isEmpty())
        return;

    if (!d->out_messages_unsent && d->out_messages_index == d->out_messages.size()-1){
        d->out_messages << plainTextEdit_INPUT->toPlainText();
        d->out_messages_unsent = true;
        d->out_messages_index++;
    }

    if (d->out_messages.at(d->out_messages_index) != plainTextEdit_INPUT->toPlainText())
        d->out_messages[d->out_messages_index] = plainTextEdit_INPUT->toPlainText();

    if (d->out_messages_index >= 1)
        d->out_messages_index--;

    plainTextEdit_INPUT->setPlainText(d->out_messages.at(d->out_messages_index));
}

void HubFrame::slotShowSearchBar(){
    Q_D(HubFrame);
    d->chatSearch->activate();
}

void HubFrame::slotHideSearchBar(){
    Q_D(HubFrame);
    d->chatSearch->dismiss();
}

void HubFrame::slotFilterTextChanged(){
    QString text = lineEdit_FILTER->text();

    Q_D(HubFrame);

    if (!text.isEmpty()){
        if (!d->proxy){
            d->proxy = new UserListProxyModel();
            d->proxy->setDynamicSortFilter(true);
            d->proxy->setSourceModel(d->model);
        }

        bool isRegExp = false;

        if (text.startsWith("##")){
            isRegExp = true;
            text.remove(0, 2);
        }

        if (!isRegExp){
            d->proxy->setFilterFixedString(text);
            d->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        }
        else{
            d->proxy->setFilterRegExp(text);
            d->proxy->setFilterCaseSensitivity(Qt::CaseSensitive);
        }

        d->proxy->setFilterKeyColumn(comboBox_COLUMNS->currentIndex());

        if (treeView_USERS->model() != d->proxy)
            treeView_USERS->setModel(d->proxy);
    }
    else if (treeView_USERS->model() != d->model)
        treeView_USERS->setModel(d->model);

    if (comboBox_COLUMNS->hasFocus())
        lineEdit_FILTER->setFocus();
}

void HubFrame::slotSmile(){
    if (!(WBGET(WB_APP_ENABLE_EMOTICON) && EmoticonFactory::getInstance()))
        return;

    if (WBGET(WB_CHAT_USE_SMILE_PANEL)){
        frame_SMILES->setVisible(!frame_SMILES->isVisible());
    }
    else {
        EmoticonDialog *dialog = new EmoticonDialog(this);

        if (dialog->exec() == QDialog::Accepted) {

            QString smiley = dialog->getEmoticonText();

            if (!smiley.isEmpty()) {

                smiley.replace("&lt;", "<");
                smiley.replace("&gt;", ">");
                smiley.replace("&amp;", "&");
                smiley.replace("&apos;", "\'");
                smiley.replace("&quot;", "\"");

                smiley += " ";

                plainTextEdit_INPUT->textCursor().insertText(smiley);
                plainTextEdit_INPUT->setFocus();
            }
        }

        delete dialog;
    }
}

void HubFrame::slotSmileClicked(){
    EmoticonLabel *lbl = qobject_cast<EmoticonLabel* >(sender());

    if (!lbl)
        return;

    QString smiley = lbl->toolTip();

    if (!smiley.isEmpty()) {

        smiley.replace("&lt;", "<");
        smiley.replace("&gt;", ">");
        smiley.replace("&amp;", "&");
        smiley.replace("&apos;", "\'");
        smiley.replace("&quot;", "\"");

        smiley += " ";

        plainTextEdit_INPUT->textCursor().insertText(smiley);
        plainTextEdit_INPUT->setFocus();
    }

    if (WBGET(WB_CHAT_HIDE_SMILE_PANEL))
        frame_SMILES->setVisible(false);
}

void HubFrame::slotSmileContextMenu(){
    QMenu *m = new QMenu(this);

    for (const auto &f : QDir(WulforUtil::getInstance()->getEmoticonsPath())
                              .entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)){
        if (!f.isEmpty()){
            QAction * act = m->addAction(f);
            act->setCheckable(true);

            if (f == WSGET(WS_APP_EMOTICON_THEME)){
                act->setChecked(false);
                act->setChecked(true);
            }
        }
    }

    QAction *a = m->exec(QCursor::pos());

    if (a && a->isChecked())
        WSSET(WS_APP_EMOTICON_THEME, a->text());

    m->deleteLater();
}

void HubFrame::slotInputTextChanged(){
#ifdef USE_ASPELL
    PMWindow *p = qobject_cast<PMWindow*>(sender());
    QTextEdit *plainTextEdit_INPUT = (p)? qobject_cast<QTextEdit*>(p->inputWidget()) : this->plainTextEdit_INPUT;

    if (!plainTextEdit_INPUT)
        return;
    QString line = plainTextEdit_INPUT->toPlainText();

    if (line.isEmpty() || !SpellCheck::getInstance())
        return;

    SpellCheck *sp = SpellCheck::getInstance();
    QStringList words = line.split(QRegExp("\\W+"), WULFOR_SKIP_EMPTY);

    if (words.isEmpty())
        return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextCursor c = plainTextEdit_INPUT->textCursor();

    QTextEdit::ExtraSelection selection;
    selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    selection.format.setUnderlineColor(AppTheme::errorColor());

    bool ok = false;
    while (!words.empty()){
        QString s = words.takeLast();

        if ((s.toLongLong(&ok) && ok) || !QUrl(s).scheme().isEmpty())
            continue;

        if (plainTextEdit_INPUT->find(s, QTextDocument::FindBackward) && !sp->ok(s)){
            selection.cursor = plainTextEdit_INPUT->textCursor();
            extraSelections.append(selection);
        }
    }

    plainTextEdit_INPUT->setTextCursor(c);
    plainTextEdit_INPUT->setExtraSelections(extraSelections);
#endif
}

void HubFrame::slotInputContextMenu(){
    PMWindow *p = qobject_cast<PMWindow*>(sender());
    QTextEdit *plainTextEdit_INPUT = (p)? qobject_cast<QTextEdit*>(p->inputWidget()) : this->plainTextEdit_INPUT;

    if (!plainTextEdit_INPUT)
        return;

    QMenu *m = plainTextEdit_INPUT->createStandardContextMenu();

#ifndef USE_ASPELL
    m->exec(QCursor::pos());
    m->deleteLater();
#else
    if (SpellCheck::getInstance()) {
        SpellCheck *sp = SpellCheck::getInstance();
        QTextCursor c = plainTextEdit_INPUT->cursorForPosition(plainTextEdit_INPUT->mapFromGlobal(QCursor::pos()));
        c.select(QTextCursor::WordUnderCursor);

        QString word = c.selectedText();

        if (sp->ok(word) || word.isEmpty()){
            c.clearSelection();
            m->exec(QCursor::pos());
            m->deleteLater();
        }
        else {
            QStringList list;
            sp->suggestions(word, list);

            m->addSeparator();
            QAction *add_to_dict = new QAction(tr("Add to dictionary"), m);

            m->addAction(add_to_dict);

            QMenu *ss = nullptr;
            if (!list.isEmpty()) {
                ss = new QMenu(tr("Suggestions"), this);


                for (const auto &s : list)
                    ss->addAction(s);

                m->addMenu(ss);
            }

            QAction *ret = m->exec(QCursor::pos());

            if (ret == add_to_dict)
                sp->addToDict(word);
            else if (ss && ret && ret->parent() == ss){
                c.removeSelectedText();

                c.insertText(ret->text());
            }

            m->deleteLater();

            if (ss)
                ss->deleteLater();

            slotInputTextChanged();
        }
    }
    else {
        m->exec(QCursor::pos());
        m->deleteLater();
    }
#endif
}

void HubFrame::slotStatusLinkOpen(const QString &url){
    WulforUtil::getInstance()->openUrl(url);
}

void HubFrame::slotHubMenu(QAction *res) {
    if (res && res->data().canConvert(QVariant::Int)) {//User command
        const int id = res->data().toInt();

        UserCommand uc;
        if (id == -1 || !FavoriteManager::getInstance()->getUserCommand(id, uc))
            return;

        StringMap params;
        Q_D(HubFrame);

        if (WulforUtil::getInstance()->getUserCommandParams(uc, params)) {
            d->client->getMyIdentity().getParams(params, "my", true);
            d->client->getHubIdentity().getParams(params, "hub", false);

            d->client->escapeParams(params);
            d->client->sendUserCmd(uc, params);
        }
    }
}

void HubFrame::slotSettingsChanged(const QString &key, const QString &value){
    if (key == WS_CHAT_FONT || key == WS_CHAT_ULIST_FONT)
        updateStyles();
    else if (key == WS_APP_EMOTICON_THEME){
        if (EmoticonFactory::getInstance()){
            EmoticonFactory::getInstance()->load();

            frame_SMILES->setVisible(false);

            clearLayout(frame_SMILES->layout());

            QSize sz;
            Q_UNUSED(sz);

            EmoticonFactory::getInstance()->fillLayout(frame_SMILES->layout(), sz);

            for (const auto &l : frame_SMILES->findChildren<EmoticonLabel*>())
                connect(l, SIGNAL(clicked()), this, SLOT(slotSmileClicked()));
        }
    }
    else if (key == "hubframe/chat-background-color" || key == "hubframe/change-chat-background-color")
        reloadSomeSettings();
    else if (key == WS_TRANSLATION_FILE){
        retranslateUi(this);
    }
}

void HubFrame::slotBoolSettingsChanged(const QString &key, int value){
    if (key == WB_APP_ENABLE_EMOTICON){
        bool enable = static_cast<bool>(value);

        if (enable){
            EmoticonFactory::newInstance();
            EmoticonFactory::getInstance()->load();

            frame_SMILES->setVisible(false);

            clearLayout(frame_SMILES->layout());

            QSize sz;
            Q_UNUSED(sz);

            EmoticonFactory::getInstance()->fillLayout(frame_SMILES->layout(), sz);

            for (const auto &l : frame_SMILES->findChildren<EmoticonLabel*>())
                connect(l, SIGNAL(clicked()), this, SLOT(slotSmileClicked()));

        }
        else{
            if (EmoticonFactory::getInstance())
                EmoticonFactory::deleteInstance();

            frame_SMILES->setVisible(false);

            clearLayout(frame_SMILES->layout());
        }

        toolButton_SMILE->setVisible(enable);
    }
}

void HubFrame::slotCopyHubIP(){
    Q_D(HubFrame);

    if (d->client && d->client->isConnected()){
        qApp->clipboard()->setText(_q(d->client->getIp()), QClipboard::Clipboard);
    }
}

void HubFrame::slotCopyHubTitle(){
    Q_D(HubFrame);

    if (d->client && d->client->isConnected()){
        qApp->clipboard()->setText(QString("%1 - %2").arg(_q(d->client->getHubName())).arg(_q(d->client->getHubDescription())), QClipboard::Clipboard);
    }
}

void HubFrame::slotCopyHubURL(){
    Q_D(HubFrame);

    if (d->client && d->client->isConnected()){
        qApp->clipboard()->setText(_q(d->client->getHubUrl()), QClipboard::Clipboard);
    }
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& aUser) noexcept {
    emit coreFavoriteUserAdded(_q(aUser.getUser()->getCID().toBase32()));
}

void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& aUser) noexcept {
    emit coreFavoriteUserRemoved(_q(aUser.getUser()->getCID().toBase32()));
}

void HubFrame::on(ClientListener::Connecting, Client *c) noexcept{
    Q_UNUSED(c)
    Q_D(HubFrame);

    d->quietUntilMs = QDateTime::currentMSecsSinceEpoch() + 12000;

    QString status = tr("Connecting to %1").arg(_q(d->client->getHubUrl()));

    emit coreConnecting(status);
}

void HubFrame::on(ClientListener::Connected, Client*) noexcept{
    Q_D(HubFrame);

    const qint64 until = QDateTime::currentMSecsSinceEpoch() + 8000;
    if (until > d->quietUntilMs)
        d->quietUntilMs = until;

    QString status = tr("Connected to %1").arg(_q(d->client->getHubUrl()));

    emit coreConnected(status);

    HubManager::getInstance()->registerHubUrl(_q(d->client->getHubUrl()), this);
}

void HubFrame::on(ClientListener::UserUpdated, Client*, const OnlineUser &user) noexcept{
    if (user.getIdentity().isHidden() && !WBGET(WB_SHOW_HIDDEN_USERS))
        return;

    emit coreUserUpdated(user.getUser(), user.getIdentity());
}

void HubFrame::on(ClientListener::UsersUpdated x, Client*, const OnlineUserList &list) noexcept{
    Q_UNUSED(x)
    for (const auto &it : list){
        const OnlineUser &user = *it;
        if (user.getIdentity().isHidden() && !WBGET(WB_SHOW_HIDDEN_USERS))
            continue;

        emit coreUserUpdated(user.getUser(), user.getIdentity());
    }
}

void HubFrame::on(ClientListener::UserRemoved, Client*, const OnlineUser &user) noexcept{
    if (user.getIdentity().isHidden() && !WBGET(WB_SHOW_HIDDEN_USERS))
        return;

    emit coreUserRemoved(user.getUser(), user.getIdentity());
}

void HubFrame::on(ClientListener::Redirect, Client*, const string &link) noexcept{
    if(ClientManager::getInstance()->isConnected(link)) {
        emit coreStatusMsg(tr("The hub is trying to redirect you to a hub you are already connected to. Disconnecting."));

        return;
    }

    if(BOOLSETTING(AUTO_FOLLOW))
        emit coreFollow(_q(link));
}

void HubFrame::on(ClientListener::Failed, Client*, const string &msg) noexcept{
    QString status = tr("Fail: %1...").arg(_q(msg));

    emit coreStatusMsg(status);
    emit coreFailed();
    emit coreHubUpdated();
}

void HubFrame::on(GetPassword, Client*) noexcept{
    emit corePassword();
}

void HubFrame::on(ClientListener::HubUpdated, Client*) noexcept{
    emit coreHubUpdated();
}

void HubFrame::on(ClientListener::StatusMessage, Client*, const string &msg, int) noexcept{
    QString status = QString("%1 ").arg(_q(msg));

    emit coreStatusMsg(status);

    Q_D(HubFrame);

    if (BOOLSETTING(LOG_STATUS_MESSAGES)){
        StringMap params;
        d->client->getHubIdentity().getParams(params, "hub", false);
        params["hubURL"] = d->client->getHubUrl();
        d->client->getMyIdentity().getParams(params, "my", true);
        params["message"] = msg;
        LOG(LogManager::STATUS, params);
    }
}

void HubFrame::on(ClientListener::NickTaken, Client*) noexcept{
    Q_D(HubFrame);
    QString status = tr("Sorry, but nick \"%1\" is already taken by another user.").arg(_q(d->client->getCurrentNick()));

    emit coreStatusMsg(status);
}

void HubFrame::on(ClientListener::SearchFlood, Client*, const string &str) noexcept{
    emit coreStatusMsg(tr("Search flood detected: %1").arg(_q(str)));
}
