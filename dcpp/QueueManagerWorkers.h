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

#pragma once

#include "CriticalSection.h"
#include "Streams.h"
#include "Thread.h"
#include "Util.h"

namespace dcpp {

class QueueManager;

/** Background move of finished downloads to their final path. */
class FileMover : public Thread {
public:
    FileMover() : active(false) { }
    virtual ~FileMover() { join(); }

    void moveFile(const string& source, const string& target);
    virtual int run();
private:
    typedef pair<string, string> FilePair;

    bool active;
    CriticalSection cs;
    vector<FilePair> files;
};

/** Background recheck of incomplete download temp files. */
class Rechecker : public Thread {
    struct DummyOutputStream : OutputStream {
        virtual size_t write(const void*, size_t n) { return n; }
        virtual size_t flush() { return 0; }
    };

public:
    explicit Rechecker(QueueManager* qm_) : qm(qm_), active(false) { }
    virtual ~Rechecker() { join(); }

    void add(const string& file);
    virtual int run();

private:
    QueueManager* qm;
    bool active;
    StringList files;
    CriticalSection cs;
};

} // namespace dcpp
