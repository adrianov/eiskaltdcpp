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
#include "hubframe/HubChatCmd.h"
#include "hubframe/HubChatCompose.h"
#include "hubframe/HubFrameMenu.h"
#include "PMWindow.h"
#include "WulforUtil.h"
#include "AppTheme.h"
#include "Antispam.h"
#include "HubManager.h"
#include "Notification.h"
#include "ShellCommandRunner.h"
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
#include "hubframe/HubPaneLayout.h"

#include "dcpp/LogManager.h"
#include "dcpp/ClientManagerHubGuard.h"
#include "dcpp/User.h"
#include "dcpp/UserCommand.h"
#include "dcpp/CID.h"
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

#include <exception>

#include "hubframe/HubFramePrivate.h"

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

    d->compose = new HubChatCompose(this);

    init();

    FavoriteHubEntry* entry = FavoriteManager::getInstance()->getFavoriteHubEntry(_tq(hub));

    if (entry && entry->getDisableChat())
        disableChat();

    d->client->connect();

    setAttribute(Qt::WA_DeleteOnClose);

    FavoriteManager::getInstance()->addListener(this);
}

HubFrame::~HubFrame(){
    Q_D(HubFrame);

    Menu::deleteInstance();

    treeView_USERS->setModel(nullptr);

    delete d->proxy;
    delete d->model;
    delete d->compose;

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
        else if ((isChat || isUserList) && m_e->button() == Qt::MiddleButton)
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

    if (d->panes)
        d->panes->restore();

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

    d->panes = new HubPaneLayout(this);
    d->panes->bind(splitter_2, treeView_USERS, textEdit_CHAT);

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
    toolButton_SMILE->setIcon(WICON(AppIcons::eiEMOTICON));
    AppTheme::applyControlButton(toolButton_SMILE);

    toolButton_HIDE->setIcon(WICON(AppIcons::eiEDITDELETE));

    frame_SMILES->setLayout(new FlowLayout(frame_SMILES));

    if (EmoticonFactory::getInstance())
        d->compose->rebuildPanel();

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
    d->completer->setMaxVisibleItems(10);
    plainTextEdit_INPUT->setCompleter(d->completer, d->model);

    d->panes->alignFields(lineEdit_FILTER, comboBox_COLUMNS, plainTextEdit_INPUT, toolButton_SMILE);

    slotSettingsChanged(WS_APP_EMOTICON_THEME, WSGET(WS_APP_EMOTICON_THEME));//toggle emoticon button
}

void HubFrame::initMenu(){
    Q_D(HubFrame);
    WulforUtil *WU = WulforUtil::getInstance();

    delete d->arenaMenu;

    d->arenaMenu = new QMenu(tr("Hub menu"), this);

    QAction *reconnect = new QAction(WU->getPixmap(AppIcons::eiRECONNECT), tr("Reconnect"), d->arenaMenu);
    QAction *show_wnd  = new QAction(WU->getPixmap(AppIcons::eiCHAT), tr("Show widget"), d->arenaMenu);
    QAction *addToFav  = new QAction(WU->getPixmap(AppIcons::eiFAVSERVER), tr("Add to Favorites"), d->arenaMenu);
    QMenu   *copyInfo  = new QMenu(tr("Copy"), d->arenaMenu);
    copyInfo->setIcon(WU->getPixmap(AppIcons::eiEDITCOPY));
    QAction *copyIP    = copyInfo->addAction(tr("Hub IP"));
    QAction *copyURL   = copyInfo->addAction(tr("Hub URL"));
    QAction *copyTitle = copyInfo->addAction(tr("Hub Title"));

    QAction *sep       = new QAction(d->arenaMenu);
    sep->setSeparator(true);
    QAction *close_wnd = new QAction(WU->getPixmap(AppIcons::eiEXIT), tr("Close"), d->arenaMenu);

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
    if (d->panes)
        d->panes->save();
    WISET(WI_CHAT_SORT_COLUMN, d->model->getSortColumn());
    WISET(WI_CHAT_SORT_ORDER, WulforUtil::getInstance()->sortOrderToInt(d->model->getSortOrder()));
    WSSET("hubframe/chat-background-color", textEdit_CHAT->palette().color(QPalette::Active, QPalette::Base).name());
}

void HubFrame::load(){
    WulforUtil::restoreTreeHeader(treeView_USERS->header(),
                                  QByteArray::fromBase64(WSGET(WS_CHAT_USERLIST_STATE).toUtf8()));

    treeView_USERS->sortByColumn(WIGET(WI_CHAT_SORT_COLUMN),
                                 WulforUtil::getInstance()->intToSortOrder(WIGET(WI_CHAT_SORT_ORDER)));

    reloadSomeSettings();
}

void HubFrame::reloadSomeSettings(){
    Q_D(HubFrame);

    label_USERSTATE->setVisible(WBGET(WB_USERS_STATISTICS));
    label_LAST_STATUS->setVisible(WBGET(WB_LAST_STATUS));

    QColor clr = AppTheme::chatBackground();
    if (WBGET("hubframe/change-chat-background-color", false)) {
        clr.setNamedColor(WSGET("hubframe/chat-background-color"));
        if (!clr.isValid() || AppTheme::isLegacyBackground(clr))
            clr = AppTheme::chatBackground();
    }
    if (d->panes)
        d->panes->fillChat(clr);
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
        return WICON(AppIcons::eiMESSAGE);
    else if (d->hasMessages)
        return WICON(AppIcons::eiHUBMSG);
    else
        return WICON(AppIcons::eiSERVER);
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

    if (!thirdPerson)
        d->compose->remember(msg);
}

bool HubFrame::parseForCmd(QString line, QWidget *wg){
    return HubChatCmd(this).run(line, wg);
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
    if(redirect.isEmpty())
        return;

    Q_D(HubFrame);
    const string url = _tq(redirect);
    if(ClientManagerHubGuard::hasActiveHub(url, d->client)) {
        addStatus(tr("Redirect skipped: already connected to that hub."));
        return;
    }

    // the client is dead, long live the client!
    d->client->removeListener(this);
    HubManager::getInstance()->unregisterHubUrl(_q(d->client->getHubUrl()));
    ClientManager::getInstance()->putClient(d->client);
    clearUsers();
    d->client = ClientManager::getInstance()->getClient(url);

    d->client->addListener(this);
    d->client->connect();
}

void HubFrame::updateStyles(){
    Q_D(HubFrame);
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

    if (d->panes)
        d->panes->alignFields(lineEdit_FILTER, comboBox_COLUMNS, plainTextEdit_INPUT, toolButton_SMILE);
}

void HubFrame::slotActivate(){
    plainTextEdit_INPUT->setFocus();
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
    Q_D(HubFrame);
    d->compose->nextMsg();
}

void HubFrame::prevMsg(){
    Q_D(HubFrame);
    d->compose->prevMsg();
}

void HubFrame::slotShowSearchBar(){
    Q_D(HubFrame);
    d->chatSearch->activate();
}

void HubFrame::slotHideSearchBar(){
    Q_D(HubFrame);
    d->chatSearch->dismiss();
}

void HubFrame::slotSmile(){
    Q_D(HubFrame);
    d->compose->toggleSmiles();
}

void HubFrame::slotSmileClicked(){
    Q_D(HubFrame);
    d->compose->smileClicked(sender());
}

void HubFrame::slotSmileContextMenu(){
    Q_D(HubFrame);
    d->compose->smileThemeMenu();
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
    Q_D(HubFrame);

    if (key == WS_CHAT_FONT || key == WS_CHAT_ULIST_FONT)
        updateStyles();
    else if (key == WS_APP_EMOTICON_THEME){
        if (EmoticonFactory::getInstance()){
            EmoticonFactory::getInstance()->load();
            d->compose->rebuildPanel();

            if (WBGET(WB_APP_ENABLE_EMOTICON))
                EmoticonFactory::getInstance()->addEmoticons(textEdit_CHAT->document());
        }
    }
    else if (key == "hubframe/chat-background-color" || key == "hubframe/change-chat-background-color")
        reloadSomeSettings();
    else if (key == WS_TRANSLATION_FILE){
        retranslateUi(this);
    }
}

void HubFrame::slotBoolSettingsChanged(const QString &key, int value){
    Q_D(HubFrame);

    if (key == WB_APP_ENABLE_EMOTICON){
        bool enable = static_cast<bool>(value);

        if (enable){
            EmoticonFactory::newInstance();
            EmoticonFactory::getInstance()->load();
            d->compose->rebuildPanel();
            EmoticonFactory::getInstance()->addEmoticons(textEdit_CHAT->document());
        }
        else{
            if (EmoticonFactory::getInstance())
                EmoticonFactory::deleteInstance();

            frame_SMILES->setVisible(false);
            d->compose->clearPanel();
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

