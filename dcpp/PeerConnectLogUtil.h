/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * Internal helpers shared by PeerConnectLogFail.cpp and PeerConnectLogDetail.cpp.
 */

#pragma once

#include "LogManager.h"
#include "UserConnection.h"
#include "Util.h"
#include "format.h"

namespace dcpp {
namespace PeerConnectLogUtil {

inline void logMsg(const string& msg) {
    LogManager::getInstance()->message(string("[Connect] ") + msg);
}

inline string linkInfo(const UserConnection* uc) {
    const string proto = uc->isSet(UserConnection::FLAG_NMDC) ? "NMDC" : "ADC";
    const string tls = uc->isSecure() ?
            (uc->isTrusted() ? "TLS" : "TLS untrusted") : "plain";
    string dir = "?";
    if(uc->isSet(UserConnection::FLAG_DOWNLOAD))
        dir = "download";
    else if(uc->isSet(UserConnection::FLAG_UPLOAD))
        dir = "upload";
    const string inc = uc->isSet(UserConnection::FLAG_INCOMING) ? "incoming" : "outgoing";
    string remote = uc->getRemoteIp();
    if(!uc->getPort().empty())
        remote += ":" + uc->getPort();
    string ret = str(F_("%1% %2% %3% %4% from %5%") % proto % tls % dir % inc % remote);
    if(uc->isSecure() && !uc->getCipherName().empty())
        ret += ", cipher=" + uc->getCipherName();
    return ret;
}

inline string directionDetail(const UserConnection* uc) {
    if(!uc || uc->getState() != UserConnection::STATE_DIRECTION)
        return Util::emptyString;
    string ret = str(F_("sent $Direction %1% %2%") % uc->getDirectionString() % uc->getNumber());
    if(!uc->getPeerDirection().empty())
        ret += str(F_(", peer $Direction %1% %2%") % uc->getPeerDirection() % uc->getPeerDirectionNum());
    else
        ret += _(", no peer $Direction received");
    return ret;
}

} // namespace PeerConnectLogUtil
} // namespace dcpp
