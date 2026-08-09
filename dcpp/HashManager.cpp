/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
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

#include "HashManager.h"

#include "format.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "Text.h"
#include "Util.h"

namespace dcpp {

const int64_t HashManager::MIN_BLOCK_SIZE = 64 * 1024;

HashManager::HashManager() {
    hasher.setOwner(this);
    TimerManager::getInstance()->addListener(this);
}

HashManager::~HashManager() {
    TimerManager::getInstance()->removeListener(this);
    hasher.join();
}

bool HashManager::checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) {
    Lock l(cs);

    const TTHValue* tthold = getFileTTHif(Text::toLower(aFileName));
    const TTHValue* tth = getFileTTHif(aFileName);
    if (tthold != NULL && tth == NULL) {
        TigerTree tt(MIN_BLOCK_SIZE);
        store.getTree(*tthold, tt);
        hashDone(aFileName, aTimeStamp, tt, 0, aSize);

        m_streamstore.saveTree(aFileName, tt);

        return true;
    }
    else if (!store.checkTTH(aFileName, aSize, aTimeStamp)) {
        hasher.hashFile(aFileName, aSize);
        return false;
    }
    return true;
}

TTHValue HashManager::getTTH(const string& aFileName, int64_t aSize) {
    Lock l(cs);
    const TTHValue* tth = store.getTTH(aFileName);
    if (tth == NULL) {
        hasher.hashFile(aFileName, aSize);
        throw HashException();
    }
    return *tth;
}

const TTHValue* HashManager::getFileTTHif(const string& aFileName) {
    Lock l(cs);
    return store.getTTH(aFileName);
}

bool HashManager::getTree(const TTHValue& root, TigerTree& tt) {
    Lock l(cs);
    return store.getTree(root, tt);
}

int64_t HashManager::getBlockSize(const TTHValue& root) {
    Lock l(cs);
    return store.getBlockSize(root);
}

void HashManager::hashDone(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, int64_t speed, int64_t size) {
    try {
        Lock l(cs);
        store.addFile(aFileName, aTimeStamp, tth, true);
        m_streamstore.saveTree(aFileName, tth);
    } catch (const Exception& e) {
        LogManager::getInstance()->message(str(F_("Hashing failed: %1%") % e.getError()));
        return;
    }

    fire(HashManagerListener::TTHDone(), aFileName, tth.getRoot());

    if (speed > 0) {
        LogManager::getInstance()->message(str(F_("Finished hashing: %1% (%2% at %3%/s)") % Util::addBrackets(aFileName) %
                                               Util::formatBytes(size) % Util::formatBytes(speed)));
    } else if(size >= 0) {
        LogManager::getInstance()->message(str(F_("Finished hashing: %1% (%2%)") % Util::addBrackets(aFileName) %
                                               Util::formatBytes(size)));
    } else {
        LogManager::getInstance()->message(str(F_("Finished hashing: %1%") % Util::addBrackets(aFileName)));
    }
}

HashManager::HashPauser::HashPauser() {
    resume = !HashManager::getInstance()->isHashingPaused();
}

HashManager::HashPauser::~HashPauser() {
    if(resume)
        HashManager::getInstance()->resumeHashing();
}

bool HashManager::pauseHashing() noexcept {
    Lock l(cs);
    return hasher.pause();
}

void HashManager::resumeHashing() noexcept {
    Lock l(cs);
    hasher.resume();
}

bool HashManager::isHashingPaused() const noexcept {
    Lock l(cs);
    return hasher.isPaused();
}

void HashManager::on(TimerManagerListener::Second, uint64_t tick) noexcept {
    (void)tick;
    static bool firstcycle = true;
    if (firstcycle){
        int delay = SETTING(HASHING_START_DELAY);
        SettingsManager *SM = SettingsManager::getInstance();
        if (delay > 1800){
            delay = 1800;
            SM->set(SettingsManager::HASHING_START_DELAY, delay);
        }

        if (!ShareManager::getInstance()->isRefreshing()){
            string  curFile;
            uint64_t bytesLeft;
            size_t  filesLeft = -1;
            getStats(curFile, bytesLeft, filesLeft);

            // if delay is more than -1 hashing process must be resumed
            // if there is nothing to hashing pause is not required
            if (isHashingPaused() && ((delay >= 0 && Util::getUpTime() >= delay) || filesLeft == 0)){
                resumeHashing();
                firstcycle = false;
            }
        }
    }
}

} // namespace dcpp
