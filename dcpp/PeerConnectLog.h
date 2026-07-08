/*
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "HintedUser.h"

namespace dcpp {

class OnlineUser;
class UserConnection;

namespace PeerConnectLog {

void queueAdded(const HintedUser& user);
void queueStart(const HintedUser& user);
void queueTimeout(const HintedUser& user, int errors);
void queueGiveUp(const HintedUser& user, int errors);
void queueSlotWaitGiveUp(const HintedUser& user, int slotWaits);
void queueSlotWait(const HintedUser& user, int slotWaits, int backoffMin);
void connected(const HintedUser& user, bool download);
void userOffline(const HintedUser& user);
void passiveWait(const HintedUser& user);
void passiveSkip(const HintedUser& user);
void fakeActiveRetry(const HintedUser& user);
void cachedList(const HintedUser& user, const string& listFile);
void nmdcSend(const OnlineUser& target, const string& cmd, const string& detail);
void nmdcRecv(const OnlineUser& sender, const string& summary);
void nmdcRecv(const string& hub, const string& summary);
void adcSend(const OnlineUser& target, const string& cmd, const string& detail);
void tcpOut(const string& user, const string& host, const string& port, bool secure, const string& proto);
void tcpFail(const string& user, const string& host, const string& port, const string& err);
void incomingReject(const string& nick, const string& reason);
void skip(const string& target, const string& hub, const string& reason);
void connectionFail(const UserConnection* uc, const string& err, bool protocolError);
void connectionReject(const UserConnection* uc, const string& reason);

} // namespace PeerConnectLog

} // namespace dcpp
