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

#pragma once

#include "DownloadManagerListener.h"
#include "UploadManagerListener.h"
#include "QueueManagerListener.h"
#include "Speaker.h"
#include "CriticalSection.h"
#include "Singleton.h"
#include "FinishedManagerListener.h"
#include "Util.h"
#include "User.h"
#include "MerkleTree.h"
#include "ClientManager.h"
#include "HintedUser.h"

#include <unordered_map>

namespace dcpp {

class FinishedManager : public Singleton<FinishedManager>,
        public Speaker<FinishedManagerListener>, private DownloadManagerListener, private UploadManagerListener, private QueueManagerListener
{
public:
    typedef unordered_map<string, FinishedFileItemPtr> MapByFile;
    typedef unordered_map<HintedUser, FinishedUserItemPtr, User::Hash> MapByUser;

    void getParams(const string& target, ParamMap& params);
    Lock lock();
    const MapByFile& getMapByFile(bool upload) const;
    const MapByUser& getMapByUser(bool upload) const;

    void remove(bool upload, const string& file);
    void remove(bool upload, const HintedUser& user);
    void removeAll(bool upload);
    /** Full path of a finished download by TTH (base32), if still on disk. */
    string getTarget(const string& aTTH);

    bool handlePartialRequest(const TTHValue& tth, vector<uint16_t>& outPartialInfo);
private:
    friend class Singleton<FinishedManager>;

    CriticalSection cs;
    MapByFile DLByFile, ULByFile;
    MapByUser DLByUser, ULByUser;
    /** Finished download targets keyed by TTH base32. */
    unordered_map<string, string> dlByTth;

    FinishedManager();
    virtual ~FinishedManager();

    void clearDLs();
    void clearULs();

    void onComplete(Transfer* t, bool upload, bool crc32Checked = false);

    virtual void on(DownloadManagerListener::Complete, Download* d) noexcept;
    virtual void on(DownloadManagerListener::Failed, Download* d, const string&) noexcept;

    virtual void on(UploadManagerListener::Complete, Upload* u) noexcept;
    virtual void on(UploadManagerListener::Failed, Upload* u, const string&) noexcept;

    virtual void on(QueueManagerListener::CRCChecked, Download* d) noexcept;
};

} // namespace dcpp
