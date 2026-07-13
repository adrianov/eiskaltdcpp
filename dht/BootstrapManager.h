/*
 * Copyright (C) 2009-2010 Big Muscle, http://strongdc.sourceforge.net/
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

#pragma once

#include "Constants.h"
#include "KBucket.h"
#include "dcpp/CID.h"
#include "dcpp/HttpConnection.h"
#include "dcpp/Singleton.h"

namespace dht
{

    /** Downloads a seed node list over HTTP, then contacts those nodes via UDP GET. */
    class BootstrapManager :
        public Singleton<BootstrapManager>, private HttpConnectionListener
    {
    public:
        BootstrapManager(void);
        ~BootstrapManager(void);

        /**
         * Start or retry HTTP seed download when the UDP queue is empty.
         * downloadFile may invoke Failed/Complete before returning; only one
         * HTTP attempt runs at a time (guarded by downloading / callActive).
         */
        void bootstrap();

        /** Send one UDP GET nodes dht.xml to the next queued bootstrap contact. */
        void process();

        void addBootstrapNode(const string& ip, const std::string &udpPort, const CID& targetCID, const UDPKey& udpKey);

    private:
        CriticalSection cs;

        struct BootstrapNode
        {
            string      ip;
            string      udpPort;
            CID         cid;
            UDPKey      udpKey;
        };

        deque<BootstrapNode> bootstrapNodes;
        HttpConnection httpConnection;
        string nodesXML;
        size_t nextServer;
        bool downloading;   // HTTP attempt in flight (sync or async)
        bool callActive;    // true while bootstrap() is inside downloadFile()
        bool syncFinished;  // Complete/Failed ran during callActive
        uint64_t lastAttempt;

        void finishDownload();

        // HttpConnectionListener (BootstrapManagerHttp.cpp)
        void on(HttpConnectionListener::Data, HttpConnection* conn, const uint8_t* buf, size_t len) throw();
        void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& aLine) throw();
        void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw();
    };

}
