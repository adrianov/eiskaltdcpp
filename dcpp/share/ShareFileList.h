/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "../MerkleTree.h"
#include "../typedefs.h"

#include <memory>

namespace dcpp {

class File;
class MemoryInputStream;
class ShareManager;

/**
 * Own share listing (files.xml.bz2): cache load, full/partial XML generation,
 * and the on-disk path used by browse and ADC GET.
 */
class ShareFileList {
public:
    explicit ShareFileList(ShareManager& share);
    ~ShareFileList();

    void setDirty() noexcept { xmlDirty = true; }
    void forceRefresh() noexcept { forceXmlRefresh = true; }

    bool loadCache() noexcept;
    /** Generate if needed; throws ShareException if the list file cannot be read. */
    const string& ensure();
    const string& getPath() const { return bzXmlFile; }

    MemoryInputStream* generatePartial(const string& dir, bool recurse) const;

    bool isBzList(const TTHValue& tth) const { return tth == bzXmlRoot; }
    bool isXmlList(const TTHValue& tth) const { return tth == xmlRoot; }
    const TTHValue& getXmlRoot() const { return xmlRoot; }
    const TTHValue& getBzRoot() const { return bzXmlRoot; }
    int64_t getXmlSize() const { return xmlListLen; }
    int64_t getBzSize() const { return bzXmlListLen; }

    static string diskPath();

private:
    void generate();

    ShareManager& share;
    int64_t xmlListLen;
    TTHValue xmlRoot;
    int64_t bzXmlListLen;
    TTHValue bzXmlRoot;
    std::unique_ptr<File> bzXmlRef;
    string bzXmlFile;
    bool xmlDirty;
    bool forceXmlRefresh;
    int listN;
    uint64_t lastXmlUpdate;
};

} // namespace dcpp
