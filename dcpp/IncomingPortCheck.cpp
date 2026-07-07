/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "IncomingPortCheck.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "HttpConnection.h"
#include "LogManager.h"
#include "LogManager.h"
#include "Text.h"
#include "format.h"

namespace dcpp {

IncomingPortCheck::IncomingPortCheck() :
    httpDone(false),
    httpFailed(false),
    busy(false)
{
}

void IncomingPortCheck::start() {
    if(!ClientManager::getInstance()->isActive())
        return;

    if(busy.exchange(true))
        return;

    Thread::start();
}

int IncomingPortCheck::run() noexcept {
    Thread::sleep(2000);

    const string tcp = ConnectionManager::getInstance()->getPort();
    const string tls = ConnectionManager::getInstance()->getSecurePort();

    auto probe = [this](const string& port) -> Result {
        Result r = fetchPort(port, "https://ifconfig.co/port/" + port);
        if(r != Result::Unknown)
            return r;
        return fetchPort(port, "https://ipconfig.io/port/" + port);
    };

    if(!tcp.empty())
        report(tcp, "TCP", probe(tcp));

    if(!tls.empty() && tls != tcp)
        report(tls, "TLS", probe(tls));

    busy = false;
    return 0;
}

IncomingPortCheck::Result IncomingPortCheck::fetchPort(const string& port, const string& url) {
    {
        Lock l(cs);
        response.clear();
        httpDone = false;
        httpFailed = false;
    }

    HttpConnection http;
    http.addListener(this);
    http.downloadFile(url);

    for(int i = 0; i < 300; ++i) {
        {
            Lock l(cs);
            if(httpDone || httpFailed)
                break;
        }
        Thread::sleep(100);
    }

    Lock l(cs);
    if(httpFailed || response.empty())
        return Result::Unknown;

    Result r = parseBody(response);
    if(r != Result::Unknown)
        return r;

    return Result::Unknown;
}

IncomingPortCheck::Result IncomingPortCheck::parseBody(const string& body) {
    const string lower = Text::toLower(body);
    if(Util::findSubString(lower, "\"reachable\":true") != string::npos)
        return Result::Open;
    if(Util::findSubString(lower, "\"reachable\":false") != string::npos)
        return Result::Closed;
    if(Util::findSubString(lower, "\"status\":\"open\"") != string::npos)
        return Result::Open;
    if(Util::findSubString(lower, "\"status\":\"closed\"") != string::npos ||
       Util::findSubString(lower, "\"status\":\"timeout\"") != string::npos)
        return Result::Closed;
    return Result::Unknown;
}

void IncomingPortCheck::report(const string& port, const string& label, Result result) {
    auto msg = [&](const char* fmt) {
        LogManager::getInstance()->message(string("Connectivity: ") + str(F_(fmt) % label % port));
    };
    switch(result) {
    case Result::Open:
        msg("Incoming %1% port %2% is reachable from the internet");
        break;
    case Result::Closed:
        msg("Incoming %1% port %2% is NOT reachable from the internet (check router forwarding, local firewall, and ISP port filtering)");
        break;
    default:
        msg("Could not verify incoming %1% port %2% (online check unavailable)");
        break;
    }
}

void IncomingPortCheck::on(HttpConnectionListener::Data, HttpConnection*, const uint8_t* buf, size_t len) noexcept {
    Lock l(cs);
    response.append(reinterpret_cast<const char*>(buf), len);
}

void IncomingPortCheck::on(HttpConnectionListener::Complete, HttpConnection*, const string&) noexcept {
    Lock l(cs);
    httpDone = true;
}

void IncomingPortCheck::on(HttpConnectionListener::Failed, HttpConnection*, const string&) noexcept {
    Lock l(cs);
    httpFailed = true;
}

} // namespace dcpp
