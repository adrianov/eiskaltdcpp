/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Slash-command interpreter for hub chat and PM windows (/away, /pm, aliases, …).
 */

#include "hubframe/HubChatCmd.h"

#include "HubFrame.h"
#include "HubFramePrivate.h"
#include "PMWindow.h"
#include "ShellCommandRunner.h"
#include "WulforSettings.h"
#include "WulforUtil.h"
#ifdef USE_ASPELL
#include "SpellCheck.h"
#endif

#include "dcpp/HashManager.h"
#include "dcpp/SettingsManager.h"
#include "dcpp/ShareManager.h"
#include "dcpp/Util.h"
#ifdef LUA_SCRIPT
#include "dcpp/ScriptManager.h"
#include "dcpp/Text.h"
#endif

#include <QStringList>

using namespace dcpp;

HubChatCmd::HubChatCmd(HubFrame *hub) : hub(hub)
{
}

void HubChatCmd::reply(QWidget *target, const QString &msg) const
{
    if (qobject_cast<HubFrame *>(target) == hub)
        hub->addStatus(msg);
    else if (PMWindow *pm = qobject_cast<PMWindow *>(target))
        pm->addStatus(msg);
}

void HubChatCmd::say(QWidget *target, const QString &msg, bool thirdPerson) const
{
    if (qobject_cast<HubFrame *>(target) == hub)
        hub->sendChat(msg, thirdPerson, false);
    else if (PMWindow *pm = qobject_cast<PMWindow *>(target))
        pm->sendMessage(msg, thirdPerson, false);
}

bool HubChatCmd::run(QString line, QWidget *target)
{
    HubFramePrivate *d = hub->d_func();

    QStringList list = line.split(" ", WULFOR_SKIP_EMPTY);

    if (list.isEmpty() || !line.startsWith("/"))
        return false;

    QString cmd = list.at(0);
    QString param;
    bool emptyParam = true;

    if (list.size() > 1) {
        param = list.at(1);
        emptyParam = false;
    }

    if (cmd == "/away") {
        if (Util::getAway() && emptyParam) {
            Util::setAway(false);
            Util::setManualAway(false);
            reply(target, HubFrame::tr("Away mode off"));
        } else {
            Util::setAway(true);
            Util::setManualAway(true);

            if (!emptyParam) {
                line.remove(0, 6);
                Util::setAwayMessage(line.toStdString());
            }

            reply(target, HubFrame::tr("Away mode on: ") + _q(Util::getAwayMessage()));
        }
    } else if (cmd == "/alias" && !emptyParam) {
        QStringList lex = line.split(" ", WULFOR_SKIP_EMPTY);

        if (lex.size() >= 2) {
            QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());

            if (lex.at(1) == "list") {
                reply(target, aliases.isEmpty() ? HubFrame::tr("Aliases not found.") : ("\n" + aliases));
            } else if (lex.at(1) == "purge" && lex.size() == 3) {
                QString alias = lex.at(2);
                QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);

                for (const auto &aline : alias_list) {
                    QStringList cmds = aline.split('\t', WULFOR_SKIP_EMPTY);

                    if (cmds.size() == 2 && alias == cmds.at(0)) {
                        alias_list.removeAt(alias_list.indexOf(aline));

                        QString new_aliases;
                        for (const auto &entry : alias_list)
                            new_aliases += entry + "\n";

                        WSSET(WS_CHAT_CMD_ALIASES, new_aliases.toUtf8().toBase64());
                        reply(target, HubFrame::tr("Alias removed."));
                    }
                }
            } else if (lex.size() >= 2) {
                QString raw = line;
                raw.remove(0, raw.indexOf(" ") + 1);

                if (raw.indexOf("::") <= 0) {
                    reply(target, HubFrame::tr("Invalid alias syntax."));
                } else {
                    QStringList new_cmd = raw.split("::", WULFOR_SKIP_EMPTY);

                    if (new_cmd.size() < 2 || new_cmd.at(1).isEmpty()) {
                        reply(target, HubFrame::tr("Invalid alias syntax."));
                    } else if (!aliases.contains(new_cmd.at(0) + '\t')) {
                        aliases += new_cmd.at(0) + '\t' + new_cmd.at(1) + '\n';
                        WSSET(WS_CHAT_CMD_ALIASES, aliases.toUtf8().toBase64());
                        reply(target, HubFrame::tr("Alias %1 => %2 has been added")
                                              .arg(new_cmd.at(0)).arg(new_cmd.at(1)));
                    }
                }
            }
        }
    } else if (cmd == "/kword" && !emptyParam) {
        if (list.size() < 2 || list.size() > 3)
            return false;

        enum { List = 0, Add, Remove };
        int kw_action = List;

        if (list.at(1) == QString("add"))
            kw_action = Add;
        else if (list.at(1) == QString("purge"))
            kw_action = Remove;
        else if (list.at(1) == QString("list"))
            kw_action = List;
        else {
            reply(target, HubFrame::tr("Invalid command syntax."));
            return false;
        }

        if (kw_action != List && list.size() != 3) {
            reply(target, HubFrame::tr("Invalid command syntax."));
            return false;
        }

        QStringList kwords = WVGET("hubframe/chat-keywords", QStringList()).toStringList();

        switch (kw_action) {
        case List: {
            QString str = HubFrame::tr("List of keywords:\n");
            for (const auto &s : kwords)
                str += "\t" + s + "\n";
            reply(target, str);
            break;
        }
        case Remove: {
            QString kword = list.last();
            if (kwords.contains(kword))
                kwords.removeOne(kword);
            break;
        }
        case Add: {
            QString kword = list.last();
            if (!kwords.contains(kword))
                kwords.push_back(kword);
            break;
        }
        default:
            break;
        }

        WVSET("hubframe/chat-keywords", kwords);
    } else if (cmd == "/ratio") {
        double up = static_cast<double>(SETTING(TOTAL_UPLOAD));
        double down = static_cast<double>(SETTING(TOTAL_DOWNLOAD));
        double ratio = down > 0 ? up / down : 0;

        QString ratioLine = HubFrame::tr("ratio: %1 (uploads: %2, downloads: %3)")
                                .arg(QString().setNum(ratio, 'f', 3))
                                .arg(WulforUtil::formatBytes(up))
                                .arg(WulforUtil::formatBytes(down));

        if (param.trimmed() == "show")
            say(target, ratioLine, true);
        else if (emptyParam)
            reply(target, ratioLine);
    } else if (cmd == "/rebuild") {
        HashManager::getInstance()->rebuild();
    } else if (cmd == "/refresh") {
        ShareManager::getInstance()->setDirty();
        ShareManager::getInstance()->refresh(true);
    }
#ifdef USE_ASPELL
    else if (cmd == "/aspell" && !emptyParam) {
        WBSET(WB_APP_ENABLE_ASPELL, param.trimmed() == "on");

        if (WBGET(WB_APP_ENABLE_ASPELL) && !SpellCheck::getInstance())
            SpellCheck::newInstance();
        else if (SpellCheck::getInstance())
            SpellCheck::deleteInstance();

        reply(target, HubFrame::tr("Aspell switched %1")
                          .arg(WBGET(WB_APP_ENABLE_ASPELL) ? HubFrame::tr("on") : HubFrame::tr("off")));
    }
#endif
    else if (cmd == "/back") {
        Util::setAway(false);
        reply(target, HubFrame::tr("Away mode off"));
    } else if (cmd == "/clear") {
        hub->textEdit_CHAT->setHtml("");
        reply(target, HubFrame::tr("Chat has been cleared"));
    } else if (cmd == "/close") {
        hub->close();
    } else if (cmd == "/fav") {
        hub->addAsFavorite();
    } else if (cmd == "/browse" && !emptyParam) {
        hub->browseUserFiles(d->model->CIDforNick(param, _q(d->client->getHubUrl())), false);
    } else if (cmd == "/grant" && !emptyParam) {
        hub->grantSlot(d->model->CIDforNick(param, _q(d->client->getHubUrl())));
    } else if (cmd == "/magnet" && !emptyParam) {
        WISET(WI_DEF_MAGNET_ACTION, param.toInt());
    } else if (cmd == "/info" && !emptyParam) {
        UserListItem *item = d->model->itemForNick(param, _q(d->client->getHubUrl()));
        if (item)
            reply(target, "\n" + hub->getUserInfo(item));
    } else if (cmd == "/me" && !emptyParam) {
        if (line.endsWith("\n"))
            line = line.left(line.lastIndexOf("\n"));

        // Temporary: ClientManager::privateMessage may need hub-only strip for /me.
        if (qobject_cast<HubFrame *>(target) == hub)
            line.remove(0, 4);

        say(target, line, true);
    } else if (cmd == "/pm" && !emptyParam) {
        hub->addPM(d->model->CIDforNick(param, _q(d->client->getHubUrl())), "", false, param);
    } else if (cmd == "/help" || cmd == "/?" || cmd == "/h") {
        QString out = "\n";
#ifdef USE_ASPELL
        out += HubFrame::tr("/aspell on/off - enable/disable spell checking\n");
#endif
        out += HubFrame::tr("/alias <ALIAS_NAME>::<COMMAND> - make alias /ALIAS_NAME to /COMMAND\n");
        out += HubFrame::tr("/alias purge <ALIAS_NAME> - remove alias\n");
        out += HubFrame::tr("/alias list - list all aliases\n");
        out += HubFrame::tr("/away <message> - set away-mode on/off\n");
        out += HubFrame::tr("/back - set away-mode off\n");
        out += HubFrame::tr("/browse <nick> - browse user files\n");
        out += HubFrame::tr("/clear - clear chat window\n");
        out += HubFrame::tr("/kword add <keyword> - add user-defined keyword which will be highlighted in the chat\n");
        out += HubFrame::tr("/kword purge <keyword> - remove user-defined keyword\n");
        out += HubFrame::tr("/kword list - full list of keywords which will be highlighted in the chat\n");
        out += HubFrame::tr("/magnet - default action with magnet (0-ask, 1-search, 2-download)\n");
        out += HubFrame::tr("/close - close this hub\n");
        out += HubFrame::tr("/fav - add this hub to favorites\n");
        out += HubFrame::tr("/grant <nick> - grant extra slot to user\n");
        out += HubFrame::tr("/help, /?, /h - show this help\n");
        out += HubFrame::tr("/info <nick> - show info about user\n");
        out += HubFrame::tr("/ratio [show] - show ratio [send in chat]\n");
        out += HubFrame::tr("/rebuild - rebuild hash\n");
        out += HubFrame::tr("/refresh - update own file list\n");
        out += HubFrame::tr("/me - say a third person\n");
        out += HubFrame::tr("/pm <nick> - begin private chat with user\n");
        out += HubFrame::tr("/ws param value - set gui option param in value (without value return current value of option)\n");
        out += HubFrame::tr("/dcpps param value - set core option param in value (without value return current value of option)\n");
#ifdef LUA_SCRIPT
        out += HubFrame::tr("/luafile <file> - load Lua file\n");
        out += HubFrame::tr("/lua <chunk> - execute Lua chunk\n");
#endif
        reply(target, out.trimmed());
    }
#ifdef LUA_SCRIPT
    else if (cmd == "/lua" && !emptyParam) {
        ScriptManager::getInstance()->EvaluateChunk(Text::fromT(_tq(param)));
    } else if (cmd == "/luafile" && !emptyParam) {
        ScriptManager::getInstance()->EvaluateFile(Text::fromT(_tq(param)));
    }
#endif
    else if (cmd == "/sh" && !emptyParam) {
        if (line.endsWith("\n"))
            line = line.left(line.lastIndexOf("\n"));

        line = line.remove(0, 4);

        ShellCommandRunner *sh = new ShellCommandRunner(line, target);
        QObject::connect(sh, SIGNAL(finished(bool,QString)), hub, SLOT(slotShellFinished(bool,QString)));
        d->shell_list.append(sh);
        sh->start();
    } else if (cmd == "/ws" && !emptyParam) {
        line = line.remove(0, 4);
        line.replace("\n", "");
        QString res;
        WSCMD(line, res);
        reply(target, res);
    } else if (cmd == "/dcpps" && !emptyParam) {
        line = line.remove(0, 7);
        reply(target, _q(SettingsManager::getInstance()->parseCoreCmd(_tq(line))));
    } else if (!WSGET(WS_CHAT_CMD_ALIASES).isEmpty()) {
        QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());
        QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);
        bool ok = false;

        for (const auto &aline : alias_list) {
            QStringList cmds = aline.split('\t', WULFOR_SKIP_EMPTY);

            if (cmds.size() == 2 && cmd == ("/" + cmds.at(0))) {
                run(cmds.at(1), target);
                ok = true;
            }
        }

        if (!ok)
            return ok;
    } else {
        return false;
    }

    return true;
}
