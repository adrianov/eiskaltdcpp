/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "typedefs.h"

#include "Socket.h"
#include "Singleton.h"
#include "SSL.h"

#ifndef SSL_SUCCESS
#define SSL_SUCCESS 1
#endif

namespace dcpp {

using std::unique_ptr;
using std::string;

class SSLSocketException : public SocketException
{
public:
#ifdef _DEBUG
    SSLSocketException(const string& aError) noexcept : SocketException("SSLSocketException: " + aError) { }
#else //_DEBUG
    SSLSocketException(const string& aError) noexcept : SocketException(aError) { }
#endif // _DEBUG
    SSLSocketException(int aError) noexcept : SocketException(aError) { }

    virtual ~SSLSocketException() throw() { }
};

class CryptoManager;

class SSLSocket : public Socket
{
public:
    virtual ~SSLSocket() { }

    void accept(const Socket& listeningSocket) override;
    void connect(const string& aIp, const string& aPort, const string& localPort = Util::emptyString) override;
    int read(void* aBuffer, int aBufLen) override;
    int write(const void* aBuffer, int aLen) override;
    int wait(uint32_t millis, int waitFor) override;
    void shutdown() noexcept override;
    void close() noexcept override;

    bool isSecure() const noexcept override { return true; }
    bool isTrusted() const noexcept override;
    string getCipherName() const noexcept override;
    ByteVector getKeyprint() const noexcept override;

    bool waitConnected(uint32_t millis) override;
    bool waitAccepted(uint32_t millis) override;

private:
    friend class CryptoManager;

    SSLSocket(SSL_CTX* context, Socket::Protocol proto);
    SSLSocket(const SSLSocket&);
    SSLSocket& operator=(const SSLSocket&);

    SSL_CTX* ctx;
    ssl::SSL ssl;
    Socket::Protocol nextProto;
    string serverName; // hostname for SNI (before DNS resolve)

    int checkSSL(int ret);
    bool waitWant(int ret, uint32_t millis);
};

} // namespace dcpp
