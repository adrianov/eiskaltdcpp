/*
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * Slash-command interpreter for hub chat and PM windows (/away, /pm, aliases, …).
 */

#include "hubframe/HubChatCmd.h"

#include "HubFrame.h"
#include "hubframe/HubFramePrivate.h"
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

#include <QHash>
#include <QStringList>
#include <functional>

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

bool HubChatCmd::doAway(QWidget *target, QString &line, bool emptyParam) const
{
    if (Util::getAway() && emptyParam) {
        Util::setAway(false);
        Util::setManualAway(false);
        reply(target, HubFrame::tr("Away mode off"));
        return true;
    }

    Util::setAway(true);
    Util::setManualAway(true);

    if (!emptyParam) {
        line.remove(0, 6);
        Util::setAwayMessage(line.toStdString());
    }

    reply(target, HubFrame::tr("Away mode on: ") + _q(Util::getAwayMessage()));
    return true;
}

bool HubChatCmd::doAlias(QWidget *target, const QString &line) const
{
    QStringList lex = line.split(" ", WULFOR_SKIP_EMPTY);
    if (lex.size() < 2)
        return true;

    QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());

    if (lex.at(1) == "list") {
        reply(target, aliases.isEmpty() ? HubFrame::tr("Aliases not found.") : ("\n" + aliases));
        return true;
    }

    if (lex.at(1) == "purge" && lex.size() == 3) {
        QString alias = lex.at(2);
        QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);

        for (const auto &aline : alias_list) {
            QStringList cmds = aline.split('\t', WULFOR_SKIP_EMPTY);
            if (cmds.size() != 2 || alias != cmds.at(0))
                continue;

            alias_list.removeAt(alias_list.indexOf(aline));

            QString new_aliases;
            for (const auto &entry : alias_list)
                new_aliases += entry + "\n";

            WSSET(WS_CHAT_CMD_ALIASES, new_aliases.toUtf8().toBase64());
            reply(target, HubFrame::tr("Alias removed."));
        }
        return true;
    }

    QString raw = line;
    raw.remove(0, raw.indexOf(" ") + 1);

    if (raw.indexOf("::") <= 0) {
        reply(target, HubFrame::tr("Invalid alias syntax."));
        return true;
    }

    QStringList new_cmd = raw.split("::", WULFOR_SKIP_EMPTY);
    if (new_cmd.size() < 2 || new_cmd.at(1).isEmpty()) {
        reply(target, HubFrame::tr("Invalid alias syntax."));
        return true;
    }

    if (!aliases.contains(new_cmd.at(0) + '\t')) {
        aliases += new_cmd.at(0) + '\t' + new_cmd.at(1) + '\n';
        WSSET(WS_CHAT_CMD_ALIASES, aliases.toUtf8().toBase64());
        reply(target, HubFrame::tr("Alias %1 => %2 has been added")
                          .arg(new_cmd.at(0)).arg(new_cmd.at(1)));
    }
    return true;
}

bool HubChatCmd::doKword(QWidget *target, const QStringList &list) const
{
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
    return true;
}

bool HubChatCmd::doRatio(QWidget *target, const QString &param, bool emptyParam) const
{
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

    return true;
}

bool HubChatCmd::doHelp(QWidget *target) const
{
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
    return true;
}

bool HubChatCmd::doMe(QWidget *target, QString &line) const
{
    if (line.endsWith("\n"))
        line = line.left(line.lastIndexOf("\n"));

    // Temporary: ClientManager::privateMessage may need hub-only strip for /me.
    if (qobject_cast<HubFrame *>(target) == hub)
        line.remove(0, 4);

    say(target, line, true);
    return true;
}

bool HubChatCmd::doSh(QWidget *target, QString &line) const
{
    HubFramePrivate *d = hub->d_func();

    if (line.endsWith("\n"))
        line = line.left(line.lastIndexOf("\n"));

    line = line.remove(0, 4);

    ShellCommandRunner *sh = new ShellCommandRunner(line, target);
    QObject::connect(sh, SIGNAL(finished(bool,QString)), hub, SLOT(slotShellFinished(bool,QString)));
    d->shell_list.append(sh);
    sh->start();
    return true;
}

bool HubChatCmd::doAliasExpand(QWidget *target, const QString &cmd)
{
    if (WSGET(WS_CHAT_CMD_ALIASES).isEmpty())
        return false;

    QString aliases = QByteArray::fromBase64(WSGET(WS_CHAT_CMD_ALIASES).toUtf8());
    QStringList alias_list = aliases.split('\n', WULFOR_SKIP_EMPTY);

    for (const auto &aline : alias_list) {
        QStringList cmds = aline.split('\t', WULFOR_SKIP_EMPTY);
        if (cmds.size() == 2 && cmd == ("/" + cmds.at(0))) {
            run(cmds.at(1), target);
            return true;
        }
    }

    return false;
}

bool HubChatCmd::run(QString line, QWidget *target)
{
    HubFramePrivate *d = hub->d_func();
    QStringList list = line.split(" ", WULFOR_SKIP_EMPTY);

    if (list.isEmpty() || !line.startsWith("/"))
        return false;

    const QString cmd = list.at(0);
    const QString param = list.size() > 1 ? list.at(1) : QString();
    const bool emptyParam = list.size() <= 1;
    const QString hubUrl = _q(d->client->getHubUrl());

    using Fn = std::function<bool()>;
    QHash<QString, Fn> handlers;

    handlers.insert("/away", [&] { return doAway(target, line, emptyParam); });
    handlers.insert("/ratio", [&] { return doRatio(target, param, emptyParam); });
    handlers.insert("/rebuild", [&] {
        HashManager::getInstance()->rebuild();
        return true;
    });
    handlers.insert("/refresh", [&] {
        ShareManager::getInstance()->setDirty();
        ShareManager::getInstance()->refresh(true);
        return true;
    });
    handlers.insert("/back", [&] {
        Util::setAway(false);
        reply(target, HubFrame::tr("Away mode off"));
        return true;
    });
    handlers.insert("/clear", [&] {
        hub->textEdit_CHAT->setHtml("");
        reply(target, HubFrame::tr("Chat has been cleared"));
        return true;
    });
    handlers.insert("/close", [&] {
        hub->close();
        return true;
    });
    handlers.insert("/fav", [&] {
        hub->addAsFavorite();
        return true;
    });

    const Fn help = [&] { return doHelp(target); };
    handlers.insert("/help", help);
    handlers.insert("/?", help);
    handlers.insert("/h", help);

    if (!emptyParam) {
        handlers.insert("/alias", [&] { return doAlias(target, line); });
        handlers.insert("/kword", [&] { return doKword(target, list); });
        handlers.insert("/browse", [&] {
            hub->browseUserFiles(d->model->CIDforNick(param, hubUrl), false);
            return true;
        });
        handlers.insert("/grant", [&] {
            hub->grantSlot(d->model->CIDforNick(param, hubUrl));
            return true;
        });
        handlers.insert("/magnet", [&] {
            WISET(WI_DEF_MAGNET_ACTION, param.toInt());
            return true;
        });
        handlers.insert("/info", [&] {
            if (UserListItem *item = d->model->itemForNick(param, hubUrl))
                reply(target, "\n" + hub->getUserInfo(item));
            return true;
        });
        handlers.insert("/me", [&] { return doMe(target, line); });
        handlers.insert("/pm", [&] {
            hub->addPM(d->model->CIDforNick(param, hubUrl), "", false, param);
            return true;
        });
        handlers.insert("/sh", [&] { return doSh(target, line); });
        handlers.insert("/ws", [&] {
            line = line.remove(0, 4);
            line.replace("\n", "");
            QString res;
            WSCMD(line, res);
            reply(target, res);
            return true;
        });
        handlers.insert("/dcpps", [&] {
            line = line.remove(0, 7);
            reply(target, _q(SettingsManager::getInstance()->parseCoreCmd(_tq(line))));
            return true;
        });
#ifdef USE_ASPELL
        handlers.insert("/aspell", [&] {
            WBSET(WB_APP_ENABLE_ASPELL, param.trimmed() == "on");
            if (WBGET(WB_APP_ENABLE_ASPELL) && !SpellCheck::getInstance())
                SpellCheck::newInstance();
            else if (SpellCheck::getInstance())
                SpellCheck::deleteInstance();
            reply(target, HubFrame::tr("Aspell switched %1")
                              .arg(WBGET(WB_APP_ENABLE_ASPELL) ? HubFrame::tr("on") : HubFrame::tr("off")));
            return true;
        });
#endif
#ifdef LUA_SCRIPT
        handlers.insert("/lua", [&] {
            ScriptManager::getInstance()->EvaluateChunk(Text::fromT(_tq(param)));
            return true;
        });
        handlers.insert("/luafile", [&] {
            ScriptManager::getInstance()->EvaluateFile(Text::fromT(_tq(param)));
            return true;
        });
#endif
    }

    if (const Fn fn = handlers.value(cmd))
        return fn();

    return doAliasExpand(target, cmd);
}
