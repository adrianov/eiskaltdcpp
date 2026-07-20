/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "PeerConnectLog.h"
#include "PeerConnectLogUtil.h"

#include "ClientManager.h"
#include "format.h"

namespace dcpp {

using PeerConnectLogUtil::directionDetail;
using PeerConnectLogUtil::linkInfo;
using PeerConnectLogUtil::logMsg;

namespace {

string phaseName(UserConnection::States s) {
    switch(s) {
    case UserConnection::STATE_UNCONNECTED: return "disconnected";
    case UserConnection::STATE_CONNECT: return "TCP/TLS connect";
    case UserConnection::STATE_SUPNICK: return "nick exchange";
    case UserConnection::STATE_INF: return "ADC INF";
    case UserConnection::STATE_LOCK: return "Lock/PK";
    case UserConnection::STATE_DIRECTION: return "Direction";
    case UserConnection::STATE_KEY: return "Key";
    case UserConnection::STATE_GET: return "upload GET wait";
    case UserConnection::STATE_SEND: return "upload Send wait";
    case UserConnection::STATE_SND: return "download request wait";
    case UserConnection::STATE_IDLE: return "idle";
    case UserConnection::STATE_RUNNING: return "transfer";
    default: return "unknown";
    }
}

bool isPostHandshake(UserConnection::States s) {
    return s == UserConnection::STATE_SND || s == UserConnection::STATE_IDLE ||
           s == UserConnection::STATE_RUNNING || s == UserConnection::STATE_GET ||
           s == UserConnection::STATE_SEND;
}

string failHint(UserConnection::States phase, const string& err, bool protocolError, bool secure) {
    if(protocolError)
        return _("unexpected or invalid protocol message");

    const string errLower = Text::toLower(err);
    const bool closed = errLower.find("closed") != string::npos;
    const bool disconnected = errLower.find("disconnect") != string::npos;
    const bool ssl = err.find("SSL") != string::npos;

    if(phase == UserConnection::STATE_DIRECTION) {
        if(ssl)
            return _("TLS error during $Direction; try plain/TLS port or trust settings");
        return _("peer closed during $Direction (conflicting roles, multi-hub race, or remote client gave up)");
    }

    if(ssl)
        return _("TLS error; try the other port (plain vs TLS) or check TLS settings");

    if(closed || disconnected) {
        if(isPostHandshake(phase))
            return _("peer closed after handshake (no upload slot, file rejected, or client disconnects when queued)");
        if(phase == UserConnection::STATE_CONNECT)
            return secure ?
                    _("peer closed during TLS connect (wrong port, blocked, or plain-only client?)") :
                    _("peer closed TCP before handshake (TLS-only client, blocked port, or bad address?)");
        if(phase >= UserConnection::STATE_SUPNICK && phase <= UserConnection::STATE_KEY)
            return _("peer closed during handshake (TLS mismatch, version mismatch, or rejected connection)");
    }
    return Util::emptyString;
}

} // namespace

namespace PeerConnectLog {

void connectionFail(const UserConnection* uc, const string& err, bool protocolError) {
    if(!uc || !uc->getUser())
        return;

    const UserConnection::States phase = uc->getState();
    const string nick = ClientManager::getInstance()->getNickOrCid(uc->getHintedUser());
    const string what = isPostHandshake(phase) ?
            string(_("connection lost")) : string(_("handshake failed"));
    string msg = str(F_("%1% during %2% (%3%): %4%") % what % phaseName(phase) % linkInfo(uc) % err);
    const string dirDetail = directionDetail(uc);
    if(!dirDetail.empty())
        msg += " [" + dirDetail + "]";
    const string hint = failHint(phase, err, protocolError, uc->isSecure());
    if(!hint.empty())
        msg += str(F_(" — %1%") % hint);
    logMsg(str(F_("%1% on %2%: %3%") % nick % uc->getHubUrl() % msg));
}

} // namespace PeerConnectLog

} // namespace dcpp
