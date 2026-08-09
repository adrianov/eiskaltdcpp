/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "hash/HashWorker.h"

#include "File.h"
#include "format.h"
#include "HashManager.h"
#include "LogManager.h"
#include "SFVReader.h"
#include "SettingsManager.h"
#include "Text.h"
#include "Util.h"
#include "ZUtils.h"
#include "hash/HashFileGate.h"
#include "hash/StreamStore.h"

#include <algorithm>
#include <memory>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace dcpp {

#ifdef _WIN32
namespace { const size_t HASH_BUF_SIZE = 256 * 1024; }
#endif

void HashWorker::hashFile(const string& fileName, int64_t size) noexcept {
    Lock l(cs);
    if(w.emplace(fileName, size).second) {
        if(paused > 0)
            paused = 1;
        else
            s.signal();
    }
}

void HashWorker::deferFile(const string& fileName, int64_t size) noexcept {
    if(fileName.empty() || size < 0)
        return;
    Lock l(cs);
    if(w.find(fileName) != w.end())
        return;
    for(auto& d: deferred) {
        if(d.first == fileName) {
            d.second = size;
            return;
        }
    }
    deferred.emplace_back(fileName, size);
}

bool HashWorker::promoteDeferred() noexcept {
    Lock l(cs);
    if(deferred.empty() || !w.empty())
        return false;
    for(auto& d: deferred)
        w.emplace(d.first, d.second);
    deferred.clear();
    return !w.empty();
}

bool HashWorker::pause() noexcept {
    Lock l(cs);
    paused = 1;
    return true;
}

void HashWorker::resume() noexcept {
    Lock l(cs);
    while(paused > 0) {
        paused = 0;
        s.signal();
    }
}

bool HashWorker::isPaused() const noexcept {
    Lock l(cs);
    return paused > 0;
}

void HashWorker::stopHashing(const string& baseDir) {
    Lock l(cs);
    for(auto i = w.begin(); i != w.end();) {
        if(Util::strnicmp(baseDir, i->first, baseDir.length()) == 0)
            w.erase(i++);
        else
            ++i;
    }
    for(auto i = deferred.begin(); i != deferred.end();) {
        if(Util::strnicmp(baseDir, i->first, baseDir.length()) == 0)
            i = deferred.erase(i);
        else
            ++i;
    }
}

void HashWorker::getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const {
    Lock l(cs);
    curFile = currentFile;
    filesLeft = w.size() + deferred.size();
    if(running)
        filesLeft++;
    bytesLeft = currentSize;
    for(auto& i: w)
        bytesLeft += i.second;
    for(auto& d: deferred)
        bytesLeft += d.second;
}

void HashWorker::instantPause() {
    bool wait = false;
    {
        Lock l(cs);
        if(paused > 0)
            wait = true;
    }
    if(wait)
        s.wait();
}

int HashWorker::run() {
    setThreadPriority(Thread::IDLE);
    uint8_t* buf = NULL;
    bool virtualBuf = true;
    string fname;
    bool last = false;
    pause();
    for(;;) {
        if(w.empty()) {
            bool waitDefer = false;
            {
                Lock l(cs);
                waitDefer = !deferred.empty();
            }
            if(waitDefer && !stop) {
                Thread::sleep(2000);
                if(promoteDeferred())
                    s.signal();
            }
            if(w.empty())
                s.wait();
        }

        if(stop)
            break;
        if(rebuild) {
            if(owner)
                owner->doRebuild();
            rebuild = false;
            LogManager::getInstance()->message(_("Hash database rebuilt"));
            continue;
        }

        int64_t queuedSize = 0;
        {
            Lock l(cs);
            if(!w.empty()) {
                currentFile = fname = w.begin()->first;
                currentSize = queuedSize = w.begin()->second;
                w.erase(w.begin());
                last = w.empty() && deferred.empty();
            } else {
                last = deferred.empty();
                fname.clear();
            }
        }
        running = true;
        instantPause();

        if(!fname.empty()) {
            if(HashFileGate::writerOpen(fname)) {
                deferFile(fname, queuedSize);
            } else {
                int64_t size = File::getSize(fname);
#ifdef _WIN32
                if(buf == NULL) {
                    virtualBuf = true;
                    buf = (uint8_t*)VirtualAlloc(NULL, 2 * HASH_BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);
                }
                const size_t bufCap = HASH_BUF_SIZE;
#else
                static const int64_t BUF_BYTES = (SETTING(HASH_BUFFER_SIZE_MB) >= 1) ? SETTING(HASH_BUFFER_SIZE_MB) * 1024 * 1024 : 0x800000;
                static const int64_t BUF_SIZE = BUF_BYTES - (BUF_BYTES % getpagesize());
                const size_t bufCap = static_cast<size_t>(BUF_SIZE);
#endif
                if(buf == NULL) {
                    virtualBuf = false;
                    buf = new uint8_t[bufCap];
                }
                try {
                    File f(fname, File::READ, File::OPEN);
                    const int64_t startSize = f.getSize();
                    int64_t bs = max(TigerTree::calcBlockSize(startSize, 10), HashManager::MIN_BLOCK_SIZE);
                    uint64_t start = GET_TICK();
                    uint32_t timestamp = f.getLastModified();
                    TigerTree slowTTH(bs);
                    TigerTree* tth = &slowTTH;

                    CRC32Filter crc32;
                    SFVReader sfv(fname);
                    CRC32Filter* xcrc32 = 0;
                    if(sfv.hasCRC())
                        xcrc32 = &crc32;

                    TigerTree fastTTH(bs);
                    tth = &fastTTH;
                    size = startSize;

#ifdef _WIN32
                    if(!virtualBuf || !BOOLSETTING(FAST_HASH) || !fastHash(fname, buf, fastTTH, size, xcrc32)) {
#else
                    if(!BOOLSETTING(FAST_HASH) || !fastHash(fname, 0, fastTTH, size, xcrc32)) {
#endif
                        tth = &slowTTH;
                        crc32 = CRC32Filter();
                        uint64_t lastRead = GET_TICK();
                        size_t n = 0;
                        do {
                            size_t bufSize = bufCap;
                            if(SETTING(MAX_HASH_SPEED) > 0) {
                                uint64_t now = GET_TICK();
                                uint64_t minTime = n * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
                                if(lastRead + minTime > now)
                                    Thread::sleep(minTime - (now - lastRead));
                                lastRead = lastRead + minTime;
                            } else {
                                lastRead = GET_TICK();
                            }
                            n = f.read(buf, bufSize);
                            tth->update(buf, n);
                            if(xcrc32)
                                (*xcrc32)(buf, n);
                            {
                                Lock l(cs);
                                currentSize = max(static_cast<uint64_t>(currentSize - n), static_cast<uint64_t>(0));
                            }
                            instantPause();
                        } while (n > 0 && !stop);
                    }

                    f.close();
                    tth->finalize();
                    uint64_t end = GET_TICK();
                    int64_t speed = 0;
                    if(end > start)
                        speed = size * _LL(1000) / (end - start);
                    if(xcrc32 && xcrc32->getValue() != sfv.getCRC()) {
                        LogManager::getInstance()->message(str(F_("%1% not shared; calculated CRC32 does not match the one found in SFV file.") % Util::addBrackets(fname)));
                    } else if(stop) {
                        // drop partial work
                    } else if(HashFileGate::acceptHash(fname, startSize, timestamp)) {
                        if(owner)
                            owner->hashDone(fname, timestamp, *tth, speed, size);
                    } else {
                        deferFile(fname, startSize);
                    }
                } catch(const FileException& e) {
                    if(HashFileGate::writerOpen(fname))
                        deferFile(fname, queuedSize);
                    else
                        LogManager::getInstance()->message(str(F_("Error hashing %1%: %2%") % Util::addBrackets(fname) % e.getError()));
                }
            }
        }
        {
            Lock l(cs);
            currentFile.clear();
            currentSize = 0;
        }
        running = false;
        if(buf != NULL && (last || stop)) {
            if(virtualBuf) {
#ifdef _WIN32
                VirtualFree(buf, 0, MEM_RELEASE);
#endif
            } else {
                delete [] buf;
            }
            buf = NULL;
        }
    }
    return 0;
}


} // namespace dcpp
