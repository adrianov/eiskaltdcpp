/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
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

#include "SearchManager.h"
#include "SearchQuery.h"
#include "format.h"
#include "IncomingPortCheck.h"
#include "LogManager.h"
#include "ClientManager.h"

namespace dcpp {

const char* SearchManager::types[TYPE_LAST] = {
    N_("Any"),
    N_("Audio"),
    N_("Compressed"),
    N_("Document"),
    N_("Executable"),
    N_("Picture"),
    N_("Video"),
    N_("Directory"),
    N_("TTH"),
    N_("CD Image")
};
const char* SearchManager::getTypeStr(int type) {
    return _(types[type]);
}

SearchManager::SearchManager() :
    stop(false)
{
    queue.start();
}

SearchManager::~SearchManager() {
    if(socket.get()) {
        stop = true;
        socket->disconnect();
#ifdef _WIN32
        join();
#endif
    }
}

string SearchManager::normalizeWhitespace(const string& aString){
    string::size_type found = 0;
    string normalized = aString;
    while((found = normalized.find_first_of("\t\n\r", found)) != string::npos) {
        normalized[found] = ' ';
        found++;
    }
    return normalized;
}

void SearchManager::search(const string& aName, int64_t aSize, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, void* aOwner /* = NULL */) {
    string query = normalizeWhitespace(aName);
    if(aTypeMode != TYPE_TTH)
        query = SearchQuery::limitHubSearch(query);
    ClientManager::getInstance()->search(aSizeMode, aSize, aTypeMode, query, aToken, aOwner);
}

uint64_t SearchManager::search(StringList& who, const string& aName, int64_t aSize /* = 0 */, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, const StringList& aExtList, void* aOwner /* = NULL */) {
    string query = normalizeWhitespace(aName);
    if(aTypeMode != TYPE_TTH)
        query = SearchQuery::limitHubSearch(query);
    return ClientManager::getInstance()->search(who, aSizeMode, aSize, aTypeMode, query, aToken, aExtList, aOwner);
}

void SearchManager::listen() {

    disconnect();

    try {
        socket.reset(new Socket);
        socket->create(Socket::TYPE_UDP);
        socket->setBlocking(true);
        socket->setSocketOpt(SO_REUSEADDR, 1);
        port = socket->bind(Util::toString(SETTING(UDP_PORT)), SETTING(BIND_IFACE)? socket->getIfaceI4(SETTING(BIND_IFACE_NAME)).c_str() : SETTING(BIND_ADDRESS));
        start();
    } catch(...) {
        socket.reset();
        throw;
    }
}

void SearchManager::disconnect() noexcept {
    if(socket.get()) {
        stop = true;
        queue.shutdown();
        socket->disconnect();
        port.clear();
        IncomingPortCheck::getInstance()->clearKind("UDP");

        join();

        socket.reset();

        stop = false;
    }
}

#define BUFSIZE 8192
int SearchManager::run() {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[BUFSIZE]);
    int len;
    sockaddr_in remoteAddr = { 0 };

    while(!stop) {
        try {
            if (!socket.get()) {
                continue;
            }
            if(socket->wait(400, Socket::WAIT_READ) != Socket::WAIT_READ) {
                continue;
            }
            if ((len = socket->read(&buf[0], BUFSIZE, remoteAddr)) > 0) {
                onData(&buf[0], len, inet_ntoa(remoteAddr.sin_addr));
                continue;
            }
        } catch(const SocketException& e) {
            dcdebug("SearchManager::run Error: %s\n", e.getError().c_str());
        }

        bool failed = false;
        while(!stop) {
            try {
                socket->disconnect();
                socket->create(Socket::TYPE_UDP);
                socket->setBlocking(true);
                socket->bind(port, SETTING(BIND_ADDRESS));
                if(failed) {
                    LogManager::getInstance()->message(_("Search enabled again"));
                    failed = false;
                }
                break;
            } catch(const SocketException& e) {
                dcdebug("SearchManager::run Stopped listening: %s\n", e.getError().c_str());

                if(!failed) {
                    LogManager::getInstance()->message(str(F_("Search disabled: %1%") % e.getError()));
                    failed = true;
                }

                // Spin for 60 seconds
                for(auto i = 0; i < 60 && !stop; ++i) {
                    Thread::sleep(1000);
                }
            }
        }
    }
    return 0;
}

} // namespace dcpp

