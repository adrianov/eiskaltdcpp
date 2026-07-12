/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "CriticalSection.h"
#include "HttpConnectionListener.h"
#include "IncomingPortCheckListener.h"
#include "Singleton.h"
#include "Speaker.h"
#include "Thread.h"

#include <atomic>
#include <string>
#include <unordered_map>

namespace dcpp {

class HttpConnection;

/** Probes whether incoming TCP ports are reachable from the public internet. */
class IncomingPortCheck :
        public Singleton<IncomingPortCheck>,
        public Speaker<IncomingPortCheckListener>,
        private Thread,
        private HttpConnectionListener
{
public:
    enum class Result { Unknown = 0, Open = 1, Closed = 2 };

    void start();

    /** Last probe result for kind ("TCP", "TLS", "UDP", …); Unknown if never checked. */
    Result getResult(const string& kind) const;
    string getPort(const string& kind) const;

    /** Mark a port open from observed traffic (e.g. UDP datagram received). No log spam. */
    void noteOpen(const string& kind, const string& port);
    /** Forget status for one kind (e.g. UDP socket rebound). */
    void clearKind(const string& kind);

private:
    friend class Singleton<IncomingPortCheck>;

    IncomingPortCheck();
    virtual ~IncomingPortCheck() noexcept { join(); }

    int run() noexcept override;

    Result fetchPort(const string& port, const string& url);
    void report(const string& port, const string& label, Result result);
    static Result parseBody(const string& body);

    void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t*, size_t) noexcept override;
    void on(HttpConnectionListener::Complete, HttpConnection*, const string&) noexcept override;
    void on(HttpConnectionListener::Failed, HttpConnection*, const string&) noexcept override;

    mutable CriticalSection cs;
    string response;
    bool httpDone;
    bool httpFailed;
    std::atomic<bool> busy;

    std::unordered_map<string, Result> lastResult;
    std::unordered_map<string, string> lastPort;
};

} // namespace dcpp
