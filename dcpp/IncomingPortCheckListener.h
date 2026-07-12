/*
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <string>

namespace dcpp {

using std::string;

class IncomingPortCheckListener {
public:
    virtual ~IncomingPortCheckListener() { }
    template<int I> struct X { enum { TYPE = I }; };

    typedef X<0> PortResult;
    typedef X<1> Finished;

    /** kind: "TCP", "UDP", "TLS", or "DHT". result: 0=Unknown, 1=Open, 2=Closed. */
    virtual void on(PortResult, const string& /*kind*/, const string& /*port*/, int /*result*/) noexcept { }
    virtual void on(Finished) noexcept { }
};

} // namespace dcpp
