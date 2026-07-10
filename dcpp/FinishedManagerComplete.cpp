/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
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

#include "FinishedManager.h"

#include "FinishedItem.h"
#include "FinishedManagerListener.h"
#include "Download.h"
#include "Upload.h"
#include "QueueManager.h"
#include "File.h"
#include "SettingsManager.h"
#include "Util.h"

namespace dcpp {

namespace {

bool isFileListPath(const string& path) {
    if(path.empty())
        return false;

    const string listPath = Util::getListPath();
    if(!listPath.empty() && path.size() >= listPath.size() &&
       Util::stricmp(path.substr(0, listPath.size()).c_str(), listPath.c_str()) == 0)
        return true;

    if(path.size() >= 4 && Util::stricmp(path.substr(path.size() - 4).c_str(), ".xml") == 0)
        return true;
    if(path.size() >= 7 && Util::stricmp(path.substr(path.size() - 7).c_str(), ".xml.bz2") == 0)
        return true;

    return Util::stricmp(Util::getFileExt(path).c_str(), ".DcLst") == 0;
}

} // namespace

void FinishedManager::onComplete(Transfer* t, bool upload, bool crc32Checked) {
    if(!upload) {
        if(t->getType() != Transfer::TYPE_FILE)
            return;
    } else if(t->getType() != Transfer::TYPE_FILE && !(t->getType() == Transfer::TYPE_FULL_LIST && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
        return;
    }

    string file = t->getPath();
    if(!upload && isFileListPath(file))
        return;
    const HintedUser& user = t->getHintedUser();

    uint64_t milliSeconds = GET_TICK() - t->getStart();
    time_t time = GET_TIME();

    int64_t size = 0, pos = 0;
    if(!upload) {
        QueueManager::getInstance()->getSizeInfo(size, pos, file);
        if (size == -1)
            return;
    } else if(t->getType() == Transfer::TYPE_FULL_LIST) {
        file += ".xml";
        if(File::getSize(file) == -1) {
            file += ".bz2";
            if(File::getSize(file) == -1)
                return;
        }
        size = t->getSize();
    }

    Lock l(cs);

    if(!upload)
        dlByTth[t->getTTH().toBase32()] = file;

    {
        MapByFile& map = upload ? ULByFile : DLByFile;
        auto it = map.find(file);
        if(it == map.end()) {
            FinishedFileItemPtr p = new FinishedFileItem(
                        pos + t->getPos(),
                        milliSeconds,
                        time,
                        upload ? File::getSize(file) : size,
                        t->getActual(),
                        crc32Checked,
                        user
                        );
            map[file] = p;
            fire(FinishedManagerListener::AddedFile(), upload, file, p);
        } else {
            it->second->update(
                        crc32Checked ? 0 : t->getPos(),
                        milliSeconds,
                        time,
                        t->getActual(),
                        crc32Checked,
                        user
                        );
            fire(FinishedManagerListener::UpdatedFile(), upload, file, it->second);
        }
    }

    {
        MapByUser& map = upload ? ULByUser : DLByUser;
        auto it = map.find(user);
        if(it == map.end()) {
            FinishedUserItemPtr p = new FinishedUserItem(
                        t->getPos(),
                        milliSeconds,
                        time,
                        file
                        );
            map[user] = p;
            fire(FinishedManagerListener::AddedUser(), upload, user, p);
        } else {
            it->second->update(
                        t->getPos(),
                        milliSeconds,
                        time,
                        file
                        );
            fire(FinishedManagerListener::UpdatedUser(), upload, user);
        }
    }
}

void FinishedManager::on(QueueManagerListener::CRCChecked, Download* d) noexcept {
    onComplete(d, false, /*crc32Checked*/true);
}

void FinishedManager::on(DownloadManagerListener::Complete, Download* d) noexcept {
    onComplete(d, false);
}

void FinishedManager::on(DownloadManagerListener::Failed, Download* d, const string&) noexcept {
    if(d->getPos() > 0)
        onComplete(d, false);
}

void FinishedManager::on(UploadManagerListener::Complete, Upload* u) noexcept {
    onComplete(u, true);
}

void FinishedManager::on(UploadManagerListener::Failed, Upload* u, const string&) noexcept {
    if(u->getPos() > 0)
        onComplete(u, true);
}

} // namespace dcpp
