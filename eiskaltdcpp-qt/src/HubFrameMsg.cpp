/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "HubFrame.h"
#include "HubFramePrivate.h"
#include "AppTheme.h"
#include "Antispam.h"
#include "PmSpamFilter.h"
#include "MainWindow.h"
#include "Notification.h"
#include "Secretary.h"
#include "WulforSettings.h"
#include "WulforUtil.h"

#include "dcpp/ChatMessage.h"
#include "dcpp/ClientManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/Util.h"

#include <QDateTime>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace dcpp;

void HubFrame::newMsg(const VarMap &map){
    Q_D(HubFrame);
    QString output;

    QString nick = map["NICK"].toString();
    QString message = map["MSG"].toString();
    QString time = "<font color=\"" + AppTheme::chatColor(WS_CHAT_TIME_COLOR)+ "\">[" + map["TIME"].toString() + "]</font>";;
    QString color = map["CLR"].toString();
    QString msg_color = WS_CHAT_MSG_COLOR;
    QString trigger;
    const bool quiet = map["QUIET"].toBool();

    const QStringList &kwords = WVGET("hubframe/chat-keywords", QStringList()).toStringList();

    for (const auto &word : kwords){
        if (message.contains(word, Qt::CaseInsensitive)){
            msg_color = WS_CHAT_SAY_NICK;
            trigger = word;

            break;
        }
    }

    if (message.indexOf(_q(d->client->getMyNick())) >= 0){
        msg_color = WS_CHAT_SAY_NICK;
        trigger = _q(d->client->getMyNick());

        if (!quiet)
            Notification::getInstance()->showMessage(Notification::NICKSAY, getArenaTitle().left(20), nick + ": " + message);
    }

    emit new_msg(map);

    if (msg_color == WS_CHAT_SAY_NICK){
        VarMap tmap = map;
        tmap["TRIGGER"] = trigger;

        emit highlighted(tmap);
    }

    bool third = map["3RD"].toBool();

    QString nicktoout = third? ("* " + nick + " ") : ("<" + nick + "> ");

    message = HubFrame::LinkParser::parseForLinks(message, true);

    WulforUtil::getInstance()->textToHtml(nicktoout, true);

    message = "<font color=\"" + AppTheme::chatColor(msg_color) + "\">" + message + "</font>";

    output  += time;
    string info= Util::formatAdditionalInfo(map["I4"].toString().toStdString(),BOOLSETTING(USE_IP),BOOLSETTING(GET_USER_COUNTRY));

    if (!info.empty())
        output  += " <font color=\"" + AppTheme::chatColor(WS_CHAT_TIME_COLOR)+ "\">" + _q(info) + "</font>";

    output  += QString(" <a style=\"text-decoration:none\" href=\"user://%1\"><font color=\"%2\"><b>%3</b></font></a>")
               .arg(nicktoout).arg(AppTheme::chatColor(color)).arg(nicktoout.replace("\"", "&quot;"));
    output  += message;

    if (!quiet && !isVisible()){
        if (msg_color == WS_CHAT_SAY_NICK)
            d->hasHighlightMessages = true;

        d->hasMessages = true;

        MainWindow::getInstance()->redrawToolPanel();
    }

    QTextDocument *chatDoc = textEdit_CHAT->document();

    if (d->drawLine && WBGET("hubframe/unreaden-draw-line", true)){
        QString hr = "<hr />";

        int scrollbarValue = textEdit_CHAT->verticalScrollBar()->value();

        for (QTextBlock it = chatDoc->begin(); it != chatDoc->end(); it = it.next()){
            if (it.userState() == 1){
                if (it.text().isEmpty()){ // additional check that it is not message
                    QTextCursor c(it);
                    c.select(QTextCursor::BlockUnderCursor);
                    c.deleteChar(); // delete string with horizontal line

                    if (scrollbarValue > textEdit_CHAT->verticalScrollBar()->maximum())
                        scrollbarValue = textEdit_CHAT->verticalScrollBar()->maximum();

                    textEdit_CHAT->verticalScrollBar()->setValue(scrollbarValue);

                    break;
                }
            }
        }

        d->drawLine = false;

        chatDoc->lastBlock().setUserState(0); // add label for the last of the old messages

        addOutput(hr + output);
        addUserData(nick);

        if (Secretary::getInstance())
            Secretary::getInstance()->coreChatMessage(nick, output, map["MSG"].toString(), _q(d->client->getIp()));

        for (QTextBlock it = chatDoc->begin(); it != chatDoc->end(); it = it.next()){
            if(!it.userState()){
                it.setUserState(-1); // delete label for the last of the old messages

                if (it.blockNumber() < chatDoc->blockCount()-3){
                    it = it.next().next();
                    it.setUserState(1); // add label for string with horizontal line

                    it = it.previous();
                    if (it.text().isEmpty()){ // additional check that it is not message
                        QTextCursor c(it);
                        c.select(QTextCursor::BlockUnderCursor);
                        c.deleteChar(); // delete empty string above horizontal line
                    }
                }

                break;
            }
        }

        return;
    }

    addOutput(output);
    addUserData(nick);

    if (Secretary::getInstance())
        Secretary::getInstance()->coreChatMessage(nick, output, map["MSG"].toString(), _q(d->client->getIp()));
}

void HubFrame::newPm(const VarMap &map){
    Q_D(HubFrame);
    QString nick = map["NICK"].toString();
    QString message = map["MSG"].toString();
    QString time    = "<font color=\"" + AppTheme::chatColor(WS_CHAT_TIME_COLOR)+ "\">[" + map["TIME"].toString() + "]</font>";
    QString color = map["CLR"].toString();
    QString full_message;
    const bool quiet = map["QUIET"].toBool();

    if (!quiet && nick != _q(d->client->getMyNick())){
        bool show_msg = false;

        if (!d->pm.contains(map["CID"].toString()))
            show_msg = true;
        else
            show_msg = (!d->pm[map["CID"].toString()]->isVisible() || WBGET("notification/play-sound-with-active-pm", true));

        if (show_msg)
            Notification::getInstance()->showMessage(Notification::PM, nick, message);
    }

    bool third = map["3RD"].toBool();

    if (message.startsWith("/me ")){
        message.remove(0, 4);
        third = true;
    }

    nick = third? ("* " + nick + " ") : ("<" + nick + "> ");

    message = HubFrame::LinkParser::parseForLinks(message, true);

    WulforUtil::getInstance()->textToHtml(nick, true);

    message       = "<font color=\"" + AppTheme::chatColor(WS_CHAT_MSG_COLOR) + "\">" + message + "</font>";
    full_message  += time;
    string info= Util::formatAdditionalInfo(map["I4"].toString().toStdString(),BOOLSETTING(USE_IP),BOOLSETTING(GET_USER_COUNTRY));

    if (!info.empty())
        full_message += " <font color=\"" + AppTheme::chatColor(WS_CHAT_TIME_COLOR)+ "\">" + _q(info) + "</font>";

    full_message  += QString(" <a style=\"text-decoration:none\" href=\"user://%1\"><font color=\"%2\"><b>%3</b></font></a>")
                     .arg(nick).arg(AppTheme::chatColor(color)).arg(nick.replace("\"", "&quot;"));
    full_message  += message;

    WulforUtil::getInstance()->textToHtml(full_message, false);

    addPM(map["CID"].toString(), full_message, true, map["NICK"].toString(), !quiet);

    if (!map["ECHO"].toBool()){
        auto it = d->pm.find(map["CID"].toString());
        if (it != d->pm.end())
            it.value()->noteIncoming(map["MSG"].toString());
    }

    if (Secretary::getInstance())
        Secretary::getInstance()->corePrivateMsg(map["NICK"].toString(), full_message, map["MSG"].toString(), _q(d->client->getIp()));
}

void HubFrame::on(ClientListener::Message, Client*, const ChatMessage &message) noexcept{
    if (message.text.empty())
        return;

    VarMap map;
    QString msg = _q(message.text);
    bool third = false;

    if (msg.startsWith("/me ")){
        msg.remove(0, 4);

        third = true;
    }
    else
        third = message.thirdPerson;

    Q_D(HubFrame);

    map["HUBURL"] = _q(d->client->getHubUrl());

    if(message.to && message.replyTo)
    {
        //private message
        const OnlineUser *user = (message.replyTo->getUser() == ClientManager::getInstance()->getMe())?
                                 message.to : message.replyTo;

        bool isBot = user->getIdentity().isBot() || user->getUser()->isSet(User::BOT);
        bool isHub = user->getIdentity().isHub();
        bool isOp  = user->getIdentity().isOp();

        if (isHub && BOOLSETTING(IGNORE_HUB_PMS))
            return;
        else if (isBot && BOOLSETTING(IGNORE_BOT_PMS))
            return;

        CID id           = user->getUser()->getCID();
        QString nick     =  _q(message.from->getIdentity().getNick());
        bool isInSandBox = false;
        bool isEcho      = (message.from->getUser() == ClientManager::getInstance()->getMe());
        bool hasPMWindow = d->pm.contains(_q(id.toBase32()));//PMWindow is created

        if (!isEcho && PmSpamFilter::getInstance() && PmSpamFilter::getInstance()->isSpam(msg))
            return;

        if (AntiSpam::getInstance())
            isInSandBox = AntiSpam::getInstance()->isInSandBox(_q(id.toBase32()));

        if (AntiSpam::getInstance() && !isEcho){
            do {
                if (hasPMWindow)
                    break;

                if (isOp && !WBGET(WB_ANTISPAM_FILTER_OPS) && !isBot)
                    break;

                if (AntiSpam::getInstance()->isInBlack(nick))
                    return;
                else if (!(AntiSpam::getInstance()->isInWhite(nick) || AntiSpam::getInstance()->isInGray(nick))){
                    AntiSpam::getInstance()->checkUser(_q(id.toBase32()), msg, _q(d->client->getHubUrl()));

                    return;
                }
            } while (false);
        }
        else if (isEcho && isInSandBox && !hasPMWindow)
            return;

        map["NICK"]  = nick;
        map["MSG"]   = msg;
        map["TIME"]  = QDateTime::currentDateTime().toString(WSGET(WS_CHAT_TIMESTAMP));
        map["ECHO"]  = isEcho;
        map["QUIET"] = isBot || isHub || QDateTime::currentMSecsSinceEpoch() < d->quietUntilMs;

        QString color = WS_CHAT_PRIV_USER_COLOR;

        if (nick == _q(d->client->getMyNick()))
            color = WS_CHAT_PRIV_LOCAL_COLOR;
        else if (isOp)
            color = WS_CHAT_OP_COLOR;
        else if (isBot)
            color = WS_CHAT_BOT_COLOR;
        else if (isHub)
            color = WS_CHAT_STAT_COLOR;

        map["CLR"] = color;
        map["3RD"] = third;
        map["CID"] = _q(id.toBase32());
        map["I4"]  = _q(message.from->getIdentity().getIp());

        if (WBGET(WB_CHAT_REDIRECT_BOT_PMS) && isBot)
            emit coreMessage(map);
        else
            emit corePrivateMsg(map);

        if (!(isBot || isHub) && (message.from->getUser() != ClientManager::getInstance()->getMe()) && Util::getAway() && !hasPMWindow)
            ClientManager::getInstance()->privateMessage(HintedUser(user->getUser(), d->client->getHubUrl()), Util::getAwayMessage(), false);

        if (BOOLSETTING(LOG_PRIVATE_CHAT)){
            string info = Util::formatAdditionalInfo(map["I4"].toString().toStdString(),BOOLSETTING(USE_IP),BOOLSETTING(GET_USER_COUNTRY));
            QString qinfo = !info.empty() ? _q(info) : "";

            StringMap params;
            params["message"] = _tq(qinfo + "<" + nick + "> " + msg);
            params["hubNI"] = _tq(WulforUtil::getInstance()->getHubNames(id));
            params["hubURL"] = d->client->getHubUrl();
            params["userCID"] = id.toBase32();
            params["userNI"] = user->getIdentity().getNick();
            params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
            params["userI4"] = message.from->getIdentity().getIp();
            LOG(LogManager::PM, params);
        }
    }
    else
    {
        // chat message
        const OnlineUser *user = message.from;

        if (d->chatDisabled)
            return;

        if (AntiSpam::getInstance() && AntiSpam::getInstance()->isInBlack(_q(user->getIdentity().getNick())))
            return;

        const bool isBot = user->getIdentity().isBot() || user->getUser()->isSet(User::BOT);
        const bool isHub = user->getIdentity().isHub();
        const bool connectQuiet = QDateTime::currentMSecsSinceEpoch() < d->quietUntilMs;

        map["NICK"] = _q(user->getIdentity().getNick());
        map["MSG"]  = msg;
        map["TIME"] = QDateTime::currentDateTime().toString(WSGET(WS_CHAT_TIMESTAMP));
        map["QUIET"] = isHub || isBot || connectQuiet;

        QString color = WS_CHAT_USER_COLOR;

        if (isHub)
            color = WS_CHAT_STAT_COLOR;
        else if (user->getUser() == d->client->getMyIdentity().getUser())
            color = WS_CHAT_LOCAL_COLOR;
        else if (user->getIdentity().isOp())
            color = WS_CHAT_OP_COLOR;
        else if (isBot)
            color = WS_CHAT_BOT_COLOR;

        if (FavoriteManager::getInstance()->isFavoriteUser(user->getUser()))
            color = WS_CHAT_FAVUSER_COLOR;

        map["CLR"] = color;
        map["3RD"] = third;
        map["I4"]  = _q(user->getIdentity().getIp());

        emit coreMessage(map);

        if (BOOLSETTING(LOG_MAIN_CHAT)){
            string info = Util::formatAdditionalInfo(map["I4"].toString().toStdString(),BOOLSETTING(USE_IP),BOOLSETTING(GET_USER_COUNTRY));
            QString qinfo = !info.empty() ? _q(info) : "";
            QString nick  =  _q(user->getIdentity().getNick());

            StringMap params;
            params["message"] = _tq(qinfo + "<" + nick + "> " + msg);
            d->client->getHubIdentity().getParams(params, "hub", false);
            params["hubURL"] = d->client->getHubUrl();
            params["userNI"] = _tq(nick);
            params["userI4"] = user->getIdentity().getIp();
            d->client->getMyIdentity().getParams(params, "my", true);
            LOG(LogManager::CHAT, params);
        }
    }
}
