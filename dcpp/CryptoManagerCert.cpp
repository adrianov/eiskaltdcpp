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
#include "LogManager.h"
#include "ClientManager.h"

#include <openssl/pem.h>

namespace dcpp {

void CryptoManager::loadCertificates() noexcept {
    if(!BOOLSETTING(USE_TLS) || !clientContext || !clientVerContext || !serverContext || !serverVerContext)
        return;

    keyprint.clear();
    certsLoaded = false;

    const string& cert = SETTING(TLS_CERTIFICATE_FILE);
    const string& key = SETTING(TLS_PRIVATE_KEY_FILE);

    if(cert.empty() || key.empty()) {
        LogManager::getInstance()->message(_("TLS disabled, no certificate file set"));
        return;
    }

    if(File::getSize(cert) == -1 || File::getSize(key) == -1 || !checkCertificate()) {
        // Try to generate them...
        try {
            generateCertificate();
            LogManager::getInstance()->message(_("Generated new TLS certificate"));
        } catch(const CryptoException& e) {
            LogManager::getInstance()->message(str(F_("TLS disabled, failed to generate certificate: %1%") % e.getError()));
        }
    }

    if(SSL_CTX_use_certificate_file(serverContext, cert.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load certificate file"));
        return;
    }
    if(SSL_CTX_use_certificate_file(clientContext, cert.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load certificate file"));
        return;
    }

    if(SSL_CTX_use_certificate_file(serverVerContext, cert.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load certificate file"));
        return;
    }
    if(SSL_CTX_use_certificate_file(clientVerContext, cert.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load certificate file"));
        return;
    }

    if(SSL_CTX_use_PrivateKey_file(serverContext, key.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load private key"));
        return;
    }
    if(SSL_CTX_use_PrivateKey_file(clientContext, key.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load private key"));
        return;
    }

    if(SSL_CTX_use_PrivateKey_file(serverVerContext, key.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load private key"));
        return;
    }
    if(SSL_CTX_use_PrivateKey_file(clientVerContext, key.c_str(), SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        LogManager::getInstance()->message(_("Failed to load private key"));
        return;
    }

    StringList certs = File::findFiles(SETTING(TLS_TRUSTED_CERTIFICATES_PATH), "*.pem");
    StringList certs2 = File::findFiles(SETTING(TLS_TRUSTED_CERTIFICATES_PATH), "*.crt");
    certs.insert(certs.end(), certs2.begin(), certs2.end());

    for(auto& i: certs) {
        if(
                SSL_CTX_load_verify_locations(clientContext, i.c_str(), NULL) != SSL_SUCCESS ||
                SSL_CTX_load_verify_locations(clientVerContext, i.c_str(), NULL) != SSL_SUCCESS ||
                SSL_CTX_load_verify_locations(serverContext, i.c_str(), NULL) != SSL_SUCCESS ||
                SSL_CTX_load_verify_locations(serverVerContext, i.c_str(), NULL) != SSL_SUCCESS
                ) {
            LogManager::getInstance()->message("Failed to load trusted certificate from " + i);
        }
    }

    loadKeyprint(cert);

    certsLoaded = true;
}

bool CryptoManager::checkCertificate() noexcept {
    FILE* f = fopen(SETTING(TLS_CERTIFICATE_FILE).c_str(), "r");
    if(!f) {
        return false;
    }

    X509* tmpx509 = NULL;
    PEM_read_X509(f, &tmpx509, NULL, NULL);
    fclose(f);

    if(!tmpx509) {
        return false;
    }
    ssl::X509 x509(tmpx509);

    ASN1_INTEGER* sn = X509_get_serialNumber(x509);
    if(!sn || !ASN1_INTEGER_get(sn)) {
        return false;
    }

    X509_NAME* name = X509_get_subject_name(x509);
    if(!name) {
        return false;
    }
    int i = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
    if(i == -1) {
        return false;
    }
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, i);
    ASN1_STRING* str = X509_NAME_ENTRY_get_data(entry);
    if(!str) {
        return false;
    }

    unsigned char* buf = 0;
    i = ASN1_STRING_to_UTF8(&buf, str);
    if(i < 0) {
        return false;
    }
    std::string cn((char*)buf, i);
    OPENSSL_free(buf);

    if(cn != ClientManager::getInstance()->getMyCID().toBase32()) {
        return false;
    }

    ASN1_TIME* t = X509_get_notAfter(x509);
    if(t) {
        if(X509_cmp_current_time(t) < 0) {
            return false;
        }
    }
    return true;
}

const ByteVector &CryptoManager::getKeyprint() const noexcept {
    return keyprint;
}

void CryptoManager::loadKeyprint(const string& file) noexcept {
    (void)file;

    FILE* f = fopen(SETTING(TLS_CERTIFICATE_FILE).c_str(), "r");
    if(!f) {
        return;
    }

    X509* tmpx509 = NULL;
    PEM_read_X509(f, &tmpx509, NULL, NULL);
    fclose(f);

    if(!tmpx509) {
        return;
    }

    ssl::X509 x509(tmpx509);

    keyprint = ssl::X509_digest(x509, EVP_sha256());
}

} // namespace dcpp
