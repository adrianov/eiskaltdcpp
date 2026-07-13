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

#include "File.h"
#include "ClientManager.h"

#include <openssl/bn.h>
#include <openssl/pem.h>

namespace dcpp {

void CryptoManager::generateCertificate() {
    // Generate certificate using OpenSSL
    if(SETTING(TLS_PRIVATE_KEY_FILE).empty()) {
        throw CryptoException(_("No private key file chosen"));
    }
    if(SETTING(TLS_CERTIFICATE_FILE).empty()) {
        throw CryptoException(_("No certificate file chosen"));
    }

    ssl::BIGNUM bn(BN_new());
    ssl::RSA rsa(RSA_new());
    ssl::EVP_PKEY pkey(EVP_PKEY_new());
    ssl::X509_NAME nm(X509_NAME_new());
    const EVP_MD *digest = EVP_sha1();
    ssl::X509 x509ss(X509_new());
    ssl::ASN1_INTEGER serial(ASN1_INTEGER_new());

    if(!bn || !rsa || !pkey || !nm || !x509ss || !serial) {
        throw CryptoException(_("Error generating certificate"));
    }

    int days = 10;
    int keylength = 2048;

#define CHECK(n) if(!(n)) { throw CryptoException(#n); }

    // Generate key pair
    CHECK((BN_set_word(bn, RSA_F4)))
            CHECK((RSA_generate_key_ex(rsa, keylength, bn, NULL)))
            CHECK((EVP_PKEY_set1_RSA(pkey, rsa)))

            // Set CID
            CHECK((X509_NAME_add_entry_by_txt(nm, "CN", MBSTRING_ASC,
                                              (const unsigned char*)ClientManager::getInstance()->getMyCID().toBase32().c_str(), -1, -1, 0)))

            // Prepare self-signed cert
            ASN1_INTEGER_set(serial, (long)Util::rand());
    CHECK((X509_set_serialNumber(x509ss, serial)))
            CHECK((X509_set_issuer_name(x509ss, nm)))
            CHECK((X509_set_subject_name(x509ss, nm)))
            CHECK((X509_gmtime_adj(X509_get_notBefore(x509ss), 0)))
            CHECK((X509_gmtime_adj(X509_get_notAfter(x509ss), (long)60*60*24*days)))
            CHECK((X509_set_pubkey(x509ss, pkey)))

            // Sign using own private key
            CHECK((X509_sign(x509ss, pkey, digest)))

        #undef CHECK
            // Write the key and cert
    {
        File::ensureDirectory(SETTING(TLS_PRIVATE_KEY_FILE));
        FILE* f = fopen(SETTING(TLS_PRIVATE_KEY_FILE).c_str(), "w");
        if(!f) {
            return;
        }
        PEM_write_RSAPrivateKey(f, rsa, NULL, NULL, 0, NULL, NULL);
        fclose(f);
    }
    {
        File::ensureDirectory(SETTING(TLS_CERTIFICATE_FILE));
        FILE* f = fopen(SETTING(TLS_CERTIFICATE_FILE).c_str(), "w");
        if(!f) {
            File::deleteFile(SETTING(TLS_PRIVATE_KEY_FILE));
            return;
        }
        PEM_write_X509(f, x509ss);
        fclose(f);
    }
}

} // namespace dcpp
