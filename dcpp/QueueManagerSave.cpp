/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2018 Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "QueueManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "version.h"

namespace dcpp {

void QueueManager::saveQueue(bool force) noexcept {
    if(!dirty && !force)
        return;

    std::vector<CID> cids;

    try {
        Lock l(cs);

        File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
        BufferedOutputStream<false> f(&ff);

        f.write(SimpleXML::utf8Header);
        f.write(LIT("<Downloads Version=\"" VERSIONSTRING "\">\r\n"));
        string tmp;
        string b32tmp;
        for(auto& i: fileQueue.getQueue()) {
            QueueItem* qi = i.second;
            if(!qi->isSet(QueueItem::FLAG_USER_LIST) || (qi->isSet(QueueItem::FLAG_USER_LIST) && SETTING(KEEP_LISTS))) {
                f.write(LIT("\t<Download Target=\""));
                f.write(SimpleXML::escape(qi->getTarget(), tmp, true));
                f.write(LIT("\" Size=\""));
                f.write(Util::toString(qi->getSize()));
                f.write(LIT("\" Priority=\""));
                f.write(Util::toString((int)qi->getPriority()));
                f.write(LIT("\" Added=\""));
                f.write(Util::toString(qi->getAdded()));
                b32tmp.clear();
                f.write(LIT("\" TTH=\""));
                f.write(qi->getTTH().toBase32(b32tmp));
                if(!qi->getDone().empty()) {
                    f.write(LIT("\" TempTarget=\""));
                    f.write(SimpleXML::escape(qi->getTempTarget(), tmp, true));
                }
                f.write(LIT("\">\r\n"));

                for(auto& j: qi->getDone()) {
                    f.write(LIT("\t\t<Segment Start=\""));
                    f.write(Util::toString(j.getStart()));
                    f.write(LIT("\" Size=\""));
                    f.write(Util::toString(j.getSize()));
                    f.write(LIT("\"/>\r\n"));
                }

                for(auto& j: qi->sources) {
#ifdef WITH_DHT
                    if(j.isSet(QueueItem::Source::FLAG_PARTIAL)|| j.getUser().hint == "DHT")
                        continue;
#else
                    if(j.isSet(QueueItem::Source::FLAG_PARTIAL))
                        continue;
#endif

                    const CID& cid = j.getUser().user->getCID();
                    const string& hint = j.getUser().hint;

                    f.write(LIT("\t\t<Source CID=\""));
                    f.write(cid.toBase32());
                    if(!hint.empty()) {
                        f.write(LIT("\" Hub=\""));
                        f.write(hint);
                    }
                    f.write(LIT("\"/>\r\n"));

                    cids.push_back(cid);
                }

                f.write(LIT("\t</Download>\r\n"));
            }
        }

        f.write(LIT("</Downloads>\r\n"));
        f.flush();
        ff.close();
        File::deleteFile(getQueueFile());
        File::renameFile(getQueueFile() + ".tmp", getQueueFile());

        dirty = false;
    } catch(const FileException&) {
        // ...
    }
    // Put this here to avoid very many saves tries when disk is full...
    lastSave = GET_TICK();

    for(auto& cid : cids) {
        ClientManager::getInstance()->saveUser(cid);
    }
}

} // namespace dcpp
