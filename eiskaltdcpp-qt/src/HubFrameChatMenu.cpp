/***************************************************************************
 * Chat log context menu for HubFrame (nick actions, chat controls, silence).
 ***************************************************************************/

#include "HubFrame.h"
#include "HubFramePrivate.h"

#include "Antispam.h"
#include "ArenaWidgetFactory.h"
#include "ArenaWidgetManager.h"
#include "MainWindow.h"
#include "SearchFrame.h"
#include "WulforUtil.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QItemSelectionModel>
#include <QMenu>
#include <QTextCursor>
#include <QTextEdit>

using namespace dcpp;

namespace {

QString nickFromAnchor(const QString &anchor){
    QString ret = anchor;
    if (!ret.startsWith("user://"))
        return ret;

    ret.remove(0, 7);
    ret = ret.trimmed();
    if (ret.startsWith("<") && ret.endsWith(">")){
        ret.remove(0, 1);
        ret = ret.left(ret.lastIndexOf(">"));
    }
    return ret;
}

QString selectedOrAnchor(QTextEdit *editor, const QPoint &p){
    QString ret = editor->textCursor().selectedText();
    if (ret.isEmpty())
        ret = editor->anchorAt(editor->mapFromGlobal(p));
    return nickFromAnchor(ret);
}

} // namespace

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
            QString ret = selectedOrAnchor(editor, p);
            if (ret.isEmpty())
                ret = editor->textCursor().block().text();
            qApp->clipboard()->setText(ret, QClipboard::Clipboard);
            break;
        }
        case Menu::SearchText:
        {
            QString ret = selectedOrAnchor(editor, p);
            if (ret.startsWith("magnet:?")) {
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
            qApp->clipboard()->setText(nick, QClipboard::Clipboard);
            break;
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
            browseUserFiles(cid, false);
            break;
        case Menu::MatchQueue:
            browseUserFiles(cid, true);
            break;
        case Menu::PrivateMessage:
            addPM(cid, "", false);
            if (d->pm.contains(cid))
                ArenaWidgetManager::getInstance()->activate(d->pm[cid]);
            break;
        case Menu::FavoriteAdd:
            addUserToFav(cid);
            break;
        case Menu::FavoriteRem:
            delUserFromFav(cid);
            break;
        case Menu::GrantSlot:
            grantSlot(cid);
            break;
        case Menu::RemoveQueue:
            delUserFromQueue(cid);
            break;
        case Menu::SilenceUser:
            silenceUser(cid);
            break;
        case Menu::UserCommands:
            break;
        case Menu::ClearChat:
            if (pmw)
                MainWindow::getInstance()->slotChatClear();
            else
                clearChat();
            break;
        case Menu::FindInChat:
            slotShowSearchBar();
            break;
        case Menu::DisableChat:
            disableChat();
            break;
        case Menu::SelectAllChat:
            editor->selectAll();
            break;
        case Menu::ZoomInChat:
            editor->zoomIn();
            break;
        case Menu::ZoomOutChat:
            editor->zoomOut();
            break;
        case Menu::AntiSpamWhite:
            if (AntiSpam::getInstance())
                (*AntiSpam::getInstance()) << eIN_WHITE << nick;
            break;
        case Menu::AntiSpamBlack:
            if (AntiSpam::getInstance())
                (*AntiSpam::getInstance()) << eIN_BLACK << nick;
            break;
        case Menu::None:
        default:
            return;
    }
}
