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
#include "CryptoManager.h"

#include "format.h"
#include "version.h"

#include <bzlib.h>

namespace dcpp {

CryptoManager::CryptoManager()
    :
      certsLoaded(false),
      lock("EXTENDEDPROTOCOLABCABCABCABCABCABC"),
      pk("DCPLUSPLUS" VERSIONSTRING)
{
    SSL_library_init();

    clientContext.reset(SSL_CTX_new(SSLv23_client_method()));
    clientVerContext.reset(SSL_CTX_new(SSLv23_client_method()));
    serverContext.reset(SSL_CTX_new(SSLv23_server_method()));
    serverVerContext.reset(SSL_CTX_new(SSLv23_server_method()));

    if(clientContext && clientVerContext && serverContext && serverVerContext) {
        initSslContexts();
    }
}

CryptoManager::~CryptoManager() {
}

bool CryptoManager::TLSOk() const noexcept {
    return BOOLSETTING(USE_TLS) && certsLoaded && !keyprint.empty();
}

SSLSocket* CryptoManager::getClientSocket(bool allowUntrusted, Socket::Protocol proto) {
    return new SSLSocket(allowUntrusted ? clientContext : clientVerContext, proto);
}
SSLSocket* CryptoManager::getServerSocket(bool allowUntrusted) {
    return new SSLSocket(allowUntrusted ? serverContext : serverVerContext, Socket::PROTO_DEFAULT);
}


void CryptoManager::decodeBZ2(const uint8_t* is, size_t sz, string& os) {
    bz_stream bs = { 0 };

    if(BZ2_bzDecompressInit(&bs, 0, 0) != BZ_OK)
        throw CryptoException(_("Error during decompression"));

    // We assume that the files aren't compressed more than 2:1...if they are it'll work anyway,
    // but we'll have to do multiple passes...
    size_t bufsize = 2*sz;
    std::unique_ptr<char[]> buf(new char[bufsize]);

    bs.avail_in = sz;
    bs.avail_out = bufsize;
    bs.next_in = reinterpret_cast<char*>(const_cast<uint8_t*>(is));
    bs.next_out = &buf[0];

    int err;

    os.clear();

    while((err = BZ2_bzDecompress(&bs)) == BZ_OK) {
        if (bs.avail_in == 0 && bs.avail_out > 0) { // error: BZ_UNEXPECTED_EOF
            BZ2_bzDecompressEnd(&bs);
            throw CryptoException(_("Error during decompression"));
        }
        os.append(&buf[0], bufsize-bs.avail_out);
        bs.avail_out = bufsize;
        bs.next_out = &buf[0];
    }

    if(err == BZ_STREAM_END)
        os.append(&buf[0], bufsize-bs.avail_out);

    BZ2_bzDecompressEnd(&bs);

    if(err < 0) {
        // This was a real error
        throw CryptoException(_("Error during decompression"));
    }
}

string CryptoManager::keySubst(const uint8_t* aKey, size_t len, size_t n) {
    std::unique_ptr<uint8_t[]> temp(new uint8_t[len + n * 10]);

    size_t j=0;

    for(size_t i = 0; i<len; i++) {
        if(isExtra(aKey[i])) {
            temp[j++] = '/'; temp[j++] = '%'; temp[j++] = 'D';
            temp[j++] = 'C'; temp[j++] = 'N';
            switch(aKey[i]) {
            case 0: temp[j++] = '0'; temp[j++] = '0'; temp[j++] = '0'; break;
            case 5: temp[j++] = '0'; temp[j++] = '0'; temp[j++] = '5'; break;
            case 36: temp[j++] = '0'; temp[j++] = '3'; temp[j++] = '6'; break;
            case 96: temp[j++] = '0'; temp[j++] = '9'; temp[j++] = '6'; break;
            case 124: temp[j++] = '1'; temp[j++] = '2'; temp[j++] = '4'; break;
            case 126: temp[j++] = '1'; temp[j++] = '2'; temp[j++] = '6'; break;
            }
            temp[j++] = '%'; temp[j++] = '/';
        } else {
            temp[j++] = aKey[i];
        }
    }
    return string((const char*)&temp[0], j);
}

string CryptoManager::makeKey(const string& aLock) {
    if(aLock.size() < 3)
        return Util::emptyString;

    std::unique_ptr<uint8_t[]> temp(new uint8_t[aLock.length()]);
    uint8_t v1;
    size_t extra=0;

    v1 = (uint8_t)(aLock[0]^5);
    v1 = (uint8_t)(((v1 >> 4) | (v1 << 4)) & 0xff);
    temp[0] = v1;

    string::size_type i;

    for(i = 1; i<aLock.length(); i++) {
        v1 = (uint8_t)(aLock[i]^aLock[i-1]);
        v1 = (uint8_t)(((v1 >> 4) | (v1 << 4))&0xff);
        temp[i] = v1;
        if(isExtra(temp[i]))
            extra++;
    }

    temp[0] = (uint8_t)(temp[0] ^ temp[aLock.length()-1]);

    if(isExtra(temp[0])) {
        extra++;
    }

    return keySubst(&temp[0], aLock.length(), extra);
}

} // namespace dcpp
