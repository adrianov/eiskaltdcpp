/***************************************************************************
 * ClientListener / FavoriteManagerListener handlers for HubFrame.
 * Filters hidden users and Peers.cn.ru Pikachu ghost swarm from the user list.
 ***************************************************************************/

#include "HubFrame.h"
#include "HubFramePrivate.h"
#include "HubManager.h"
#include "WulforSettings.h"

#include "dcpp/ClientManager.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/LogManager.h"
#include "dcpp/PeerConnectFilter.h"
#include "dcpp/SettingsManager.h"

#include <QDateTime>

using namespace dcpp;

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

    if (PeerConnectFilter::isPikachuGhost(user.getIdentity())) {
        emit coreUserRemoved(user.getUser(), user.getIdentity());
        return;
    }

    emit coreUserUpdated(user.getUser(), user.getIdentity());
}

void HubFrame::on(ClientListener::UsersUpdated x, Client*, const OnlineUserList &list) noexcept{
    Q_UNUSED(x)
    for (const auto &it : list){
        const OnlineUser &user = *it;
        if (user.getIdentity().isHidden() && !WBGET(WB_SHOW_HIDDEN_USERS))
            continue;

        if (PeerConnectFilter::isPikachuGhost(user.getIdentity())) {
            emit coreUserRemoved(user.getUser(), user.getIdentity());
            continue;
        }

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

void HubFrame::on(ClientListener::GetPassword, Client*) noexcept{
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
