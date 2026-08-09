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
#include "hash/HashDataFile.h"

#include "File.h"
#include "HashManager.h"
#include "LogManager.h"
#include "Util.h"
#include "format.h"

#include <memory>

namespace dcpp {

string HashDataFile::path() {
    return Util::getPath(Util::PATH_USER_CONFIG) + "HashData.dat";
}

void HashDataFile::create(const string& name) {
    try {
        File dat(name, File::WRITE, File::CREATE | File::TRUNCATE);
        dat.setPos(1024 * 1024);
        dat.setEOF();
        dat.setPos(0);
        int64_t start = sizeof(start);
        dat.write(&start, sizeof(start));
    } catch (const FileException& e) {
        LogManager::getInstance()->message(str(F_("Error creating hash data file: %1%") % e.getError()));
    }
}

void HashDataFile::ensureReady() {
    Util::migrate(path());
    if (File::getSize(path()) <= static_cast<int64_t>(sizeof(int64_t))) {
        try {
            create(path());
        } catch (const FileException&) {
        }
    }
}

int64_t HashDataFile::writeLeaves(File& f, const TigerTree& tt) {
    if (tt.getLeaves().size() == 1)
        return SMALL_TREE;

    f.setPos(0);
    int64_t pos = 0;
    size_t n = sizeof(pos);
    if (f.read(&pos, n) != sizeof(pos))
        throw HashException(_("Unable to read hash data file"));

    int64_t datsz = f.getSize();
    if ((pos + (int64_t)(tt.getLeaves().size() * TTHValue::BYTES)) >= datsz) {
        f.setPos(datsz + 1024 * 1024);
        f.setEOF();
    }
    f.setPos(pos);
    dcassert(tt.getLeaves().size() > 1);
    f.write(tt.getLeaves()[0].data, (tt.getLeaves().size() * TTHValue::BYTES));
    int64_t p2 = f.getPos();
    f.setPos(0);
    f.write(&p2, sizeof(p2));
    return pos;
}

bool HashDataFile::readTree(File& f, int64_t index, int64_t size, int64_t blockSize,
                            const TTHValue& root, TigerTree& out) {
    if (index == SMALL_TREE) {
        out = TigerTree(size, blockSize, root);
        return true;
    }
    try {
        f.setPos(index);
        size_t datalen = TigerTree::calcBlocks(size, blockSize) * TTHValue::BYTES;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[datalen]);
        f.read(&buf[0], datalen);
        out = TigerTree(size, blockSize, &buf[0]);
        return out.getRoot() == root;
    } catch (const Exception&) {
        return false;
    }
}

} // namespace dcpp
