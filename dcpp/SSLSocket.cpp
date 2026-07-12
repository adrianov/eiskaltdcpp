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

#include "format.h"

#include <openssl/err.h>

namespace dcpp {

SSLSocket::SSLSocket(SSL_CTX* context, Socket::Protocol proto) : ctx(context), ssl(0), nextProto(proto) {
}

bool SSLSocket::waitWant(int ret, uint32_t millis) {
    switch(SSL_get_error(ssl, ret)) {
    case SSL_ERROR_WANT_READ:
        return wait(millis, Socket::WAIT_READ) == WAIT_READ;
    case SSL_ERROR_WANT_WRITE:
        return wait(millis, Socket::WAIT_WRITE) == WAIT_WRITE;
    default:
        checkSSL(ret);
    }
    return true;
}

int SSLSocket::checkSSL(int ret) {
    if(!ssl)
        return -1;
    if(ret > 0)
        return ret;

    // https://www.openssl.org/docs/manmaster/man3/SSL_get_error.html
    const int err = SSL_get_error(ssl, ret);
    switch(err) {
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        return -1;
    case SSL_ERROR_ZERO_RETURN:
        throw SocketException(_("Connection closed"));
    case SSL_ERROR_SYSCALL: {
        // Empty queue ⇒ EOF or errno (reset, broken pipe, …) — not a libssl reason code.
        ssl.reset();
        const unsigned long e = ERR_get_error();
        if(e != 0) {
            char errbuf[256];
            throw SSLSocketException(str(F_("SSL Error: %1% (%2%, %3%)") % ERR_error_string(e, errbuf) % ret % err));
        }
        if(ret == 0 || getLastError() == 0)
            throw SocketException(_("Connection closed"));
        throw SocketException(getLastError());
    }
    default: {
        ssl.reset();
        char errbuf[256];
        throw SSLSocketException(str(F_("SSL Error: %1% (%2%, %3%)") % ERR_error_string(ERR_get_error(), errbuf) % ret % err));
    }
    }
}

int SSLSocket::read(void* aBuffer, int aBufLen) {
    if(!ssl) {
        return -1;
    }
    int len = checkSSL(SSL_read(ssl, aBuffer, aBufLen));

    if(len > 0) {
        stats.totalDown += len;
    }
    return len;
}

int SSLSocket::write(const void* aBuffer, int aLen) {
    if(!ssl) {
        return -1;
    }
    int ret = checkSSL(SSL_write(ssl, aBuffer, aLen));
    if(ret > 0) {
        stats.totalUp += ret;
    }
    return ret;
}

int SSLSocket::wait(uint32_t millis, int waitFor) {
    if(ssl && (waitFor & Socket::WAIT_READ)) {
        /** @todo Take writing into account as well if reading is possible? */
        char c;
        if(SSL_peek(ssl, &c, 1) > 0)
            return WAIT_READ;
    }
    return Socket::wait(millis, waitFor);
}

bool SSLSocket::isTrusted() const noexcept {
    if(!ssl) {
        return false;
    }

    if(SSL_get_verify_result(ssl) != X509_V_OK) {
        return false;
    }

    X509* cert = SSL_get_peer_certificate(ssl);
    if(!cert) {
        return false;
    }
    X509_free(cert);
    return true;
}

std::string SSLSocket::getCipherName() const noexcept {
    if(!ssl)
        return Util::emptyString;

    return SSL_get_cipher_name(ssl);
}

ByteVector SSLSocket::getKeyprint() const noexcept {
    if(!ssl)
        return ByteVector();
    X509* x509 = SSL_get_peer_certificate(ssl);
    if(!x509)
        return ByteVector();

    return ssl::X509_digest(x509, EVP_sha256());
}

void SSLSocket::shutdown() noexcept {
    if(ssl)
        SSL_shutdown(ssl);
}

void SSLSocket::close() noexcept {
    if(ssl) {
        ssl.reset();
    }
    serverName.clear();
    Socket::shutdown();
    Socket::close();
}

} // namespace dcpp
