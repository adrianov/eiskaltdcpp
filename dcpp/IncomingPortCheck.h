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
#include "Singleton.h"
#include "Thread.h"

#include <atomic>

namespace dcpp {

class HttpConnection;

/** Probes whether incoming TCP ports are reachable from the public internet. */
class IncomingPortCheck :
        public Singleton<IncomingPortCheck>,
        private Thread,
        private HttpConnectionListener
{
public:
    void start();

private:
    friend class Singleton<IncomingPortCheck>;

    IncomingPortCheck();
    virtual ~IncomingPortCheck() noexcept { join(); }

    int run() noexcept override;

    enum class Result { Unknown, Open, Closed };

    Result fetchPort(const string& port, const string& url);
    void report(const string& port, const string& label, Result result);
    static Result parseBody(const string& body);

    void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t*, size_t) noexcept override;
    void on(HttpConnectionListener::Complete, HttpConnection*, const string&) noexcept override;
    void on(HttpConnectionListener::Failed, HttpConnection*, const string&) noexcept override;

    CriticalSection cs;
    string response;
    bool httpDone;
    bool httpFailed;
    std::atomic<bool> busy;
};

} // namespace dcpp
