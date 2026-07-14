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
#include "Util.h"
#include "format.h"

#include <regex>
namespace dcpp {

IncomingPortCheck::IncomingPortCheck() :
    httpDone(false),
    httpFailed(false),
    busy(false),
    stopping(false)
{
}
void IncomingPortCheck::start() {
    if(!ClientManager::getInstance()->isActive())
        return;

    if(busy.exchange(true))
        return;

    {
        Lock l(cs);
        lastResult.erase("TCP");
        lastResult.erase("TLS");
        lastPort.erase("TCP");
        lastPort.erase("TLS");
    }
    fire(IncomingPortCheckListener::PortResult(), string(), string(),
         static_cast<int>(Result::Unknown));
    Thread::start();
}

IncomingPortCheck::Result IncomingPortCheck::getResult(const string& kind) const {
    Lock l(cs);
    auto i = lastResult.find(kind);
    return i == lastResult.end() ? Result::Unknown : i->second;
}

string IncomingPortCheck::getPort(const string& kind) const {
    Lock l(cs);
    auto i = lastPort.find(kind);
    return i == lastPort.end() ? Util::emptyString : i->second;
}

void IncomingPortCheck::noteOpen(const string& kind, const string& port) {
    {
        Lock l(cs);
        if(lastResult[kind] == Result::Open && lastPort[kind] == port)
            return;
        lastResult[kind] = Result::Open;
        lastPort[kind] = port;
    }
    fire(IncomingPortCheckListener::PortResult(), kind, port, static_cast<int>(Result::Open));
}

void IncomingPortCheck::clearKind(const string& kind) {
    {
        Lock l(cs);
        lastResult.erase(kind);
        lastPort.erase(kind);
    }
    fire(IncomingPortCheckListener::PortResult(), kind, string(),
         static_cast<int>(Result::Unknown));
}

int IncomingPortCheck::run() noexcept {
    if(stopSem.wait(2000))
        return 0;
    const string tcp = ConnectionManager::getInstance()->getPort();
    const string tls = ConnectionManager::getInstance()->getSecurePort();

    auto probe = [this](const string& port) -> Result {
        Result r = fetchPort(port, "https://ifconfig.co/port/" + port);
        if(r != Result::Unknown || stopping)
            return r;
        return fetchPort(port, "https://ipconfig.io/port/" + port);
    };

    auto check = [&](const string& port, const string& label) {
        if(port.empty() || stopping)
            return;
        const Result result = probe(port);
        if(!stopping)
            report(port, label, result);
    };

    check(tcp, "TCP");
    if(tls != tcp)
        check(tls, "TLS");

    busy = false;
    if(!stopping)
        fire(IncomingPortCheckListener::Finished());
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
    for(int i = 0; i < 300 && !stopping; ++i) {
        {
            Lock l(cs);
            if(httpDone || httpFailed)
                break;
        }
        if(stopSem.wait(100))
            break;
    }

    http.removeListener(this);

    Lock l(cs);
    if(httpFailed || response.empty())
        return Result::Unknown;

    return parseBody(response);
}

IncomingPortCheck::Result IncomingPortCheck::parseBody(const string& body) {
    // Pretty or compact JSON; \s matches spaces and newlines in the body.
    static const std::regex openRe(
        R"re("reachable"\s*:\s*true|"status"\s*:\s*"open")re",
        std::regex::icase);
    static const std::regex closedRe(
        R"re("reachable"\s*:\s*false|"status"\s*:\s*"(?:closed|timeout)")re",
        std::regex::icase);

    if(std::regex_search(body, openRe))
        return Result::Open;
    if(std::regex_search(body, closedRe))
        return Result::Closed;
    return Result::Unknown;
}

void IncomingPortCheck::report(const string& port, const string& label, Result result) {
    {
        Lock l(cs);
        lastResult[label] = result;
        lastPort[label] = port;
    }

    fire(IncomingPortCheckListener::PortResult(), label, port, static_cast<int>(result));

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
