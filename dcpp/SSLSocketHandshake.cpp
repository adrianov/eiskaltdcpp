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

#include "stdinc.h"
#include "SSLSocket.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace dcpp {

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
static const unsigned char alpn_protos_nmdc[] = {
	4, 'n', 'm', 'd', 'c',
};
static const unsigned char alpn_protos_adc[] = {
	3, 'a', 'd', 'c',
};
#endif

#if OPENSSL_VERSION_NUMBER < 0x10002000L
static inline int SSL_is_server(SSL *s)
{
    return s->server;
}
#endif

/** True when host is a literal IPv4/IPv6 address (no SNI). */
static bool isLiteralIp(const string& host) {
    unsigned char buf[16];
    return inet_pton(AF_INET, host.c_str(), buf) == 1
        || inet_pton(AF_INET6, host.c_str(), buf) == 1;
}

void SSLSocket::connect(const string& aIp, const string& aPort, const string& localPort) {
    // Keep the name before Socket::connect resolves it to an address — CDNs need SNI.
    serverName = aIp;
    Socket::connect(aIp, aPort, localPort);
}

bool SSLSocket::waitConnected(uint32_t millis) {
    if(!ssl) {
        if(!Socket::waitConnected(millis)) {
            return false;
        }
        ssl.reset(SSL_new(ctx));
        if(!ssl)
            checkSSL(-1);

        checkSSL(SSL_set_fd(ssl, sock));

        if(!serverName.empty() && !isLiteralIp(serverName))
            SSL_set_tlsext_host_name(ssl, serverName.c_str());

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
        if (nextProto == Socket::PROTO_NMDC) {
            SSL_set_alpn_protos(ssl, alpn_protos_nmdc, sizeof(alpn_protos_nmdc));
        } else if (nextProto == Socket::PROTO_ADC) {
            SSL_set_alpn_protos(ssl, alpn_protos_adc, sizeof(alpn_protos_adc));
        }
#endif
    }

    if(SSL_is_init_finished(ssl)) {
        return true;
    }

    while(true) {
        int ret = SSL_is_server(ssl)?SSL_accept(ssl):SSL_connect(ssl);
        if(ret == 1) {
            dcdebug("Connected to SSL server using %s as %s\n",
                    SSL_get_cipher(ssl), SSL_is_server(ssl) ? "server" : "client");

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
            if (SSL_is_server(ssl)) return true;

            const unsigned char* protocol = 0;
            unsigned int len = 0;
            SSL_get0_alpn_selected(ssl, &protocol, &len);
            if (len != 0)
            {
                if (len == 3 && !memcmp(protocol, "adc", len))
                    proto = PROTO_ADC;
                else if (len == 4 && !memcmp(protocol, "nmdc", len))
                    proto = PROTO_NMDC;
                dcdebug("ALPN negotiated %.*s (%d)\n", len, protocol, proto);
            }
#endif
            return true;
        }
        if(!waitWant(ret, millis)) {
            return false;
        }
    }
}

void SSLSocket::accept(const Socket& listeningSocket) {
    Socket::accept(listeningSocket);

    waitAccepted(0);
}

bool SSLSocket::waitAccepted(uint32_t millis) {
    if(!ssl) {
        if(!Socket::waitAccepted(millis)) {
            return false;
        }
        ssl.reset(SSL_new(ctx));
        if(!ssl)
            checkSSL(-1);

        checkSSL(SSL_set_fd(ssl, sock));
    }

    if(SSL_is_init_finished(ssl)) {
        return true;
    }

    while(true) {
        int ret = SSL_accept(ssl);
        if(ret == 1) {
            dcdebug("Connected to SSL client using %s\n", SSL_get_cipher(ssl));
            return true;
        }
        if(!waitWant(ret, millis)) {
            return false;
        }
    }
}

} // namespace dcpp
