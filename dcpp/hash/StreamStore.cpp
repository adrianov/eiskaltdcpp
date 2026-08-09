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
#include "hash/StreamStore.h"

#include "File.h"

#include <memory>

#ifdef USE_XATTR
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/xattr.h>
#endif

namespace dcpp {

const string StreamStore::g_streamName(".gltth");

inline void StreamStore::setCheckSum(TTHStreamHeader& p_header) {
    p_header.magic = g_MAGIC;
    uint32_t l_sum = 0;

    for (size_t i = 0; i < sizeof(TTHStreamHeader) / sizeof(uint32_t); i++)
        l_sum ^= ((uint32_t*) & p_header)[i];

    p_header.checksum ^= l_sum;
}

inline bool StreamStore::validateCheckSum(const TTHStreamHeader& p_header) {
    if (p_header.magic != g_MAGIC)
        return false;

    uint32_t l_sum = 0;

    for (size_t i = 0; i < sizeof(TTHStreamHeader) / sizeof(uint32_t); i++)
        l_sum ^= ((uint32_t*) & p_header)[i];

    return (l_sum == 0);
}

#ifdef USE_XATTR
static const uint64_t SIGNIFIC_VALUE    = 10000000;
static const uint64_t NTFS_TIME_OFFSET  = ((uint64_t)(369 * 365 + 89) * 24 * 3600 * SIGNIFIC_VALUE);

static uint64_t getTimeStamp(const string &fname){
    struct stat st;

    /* WARNING: this is not completely portable conversion!
       For more information about portable conversion of linux time to windows filetime see
       ntfs-3g_ntfsprogs/include/ntfs-3g/ntfstime.h from NTFS-3G sources. */
    if (::stat(fname.c_str(), &st) == 0)
        return (uint64_t)st.st_mtime * SIGNIFIC_VALUE + NTFS_TIME_OFFSET + st.st_mtim.tv_nsec/100;

    return 0;
}

#endif // USE_XATTR

bool StreamStore::loadTree(const string& p_filePath, TigerTree &tree, int64_t p_aFileSize)
{
#ifdef USE_XATTR
    const int64_t fileSize  = (p_aFileSize == -1) ? File::getSize(p_filePath) : p_aFileSize;
    const size_t hdrSz      = sizeof(TTHStreamHeader);
    const size_t totalSz    = ATTR_MAX_VALUELEN;
    const size_t blockSize  = totalSz;

    std::unique_ptr<uint8_t[]> buf(new uint8_t[blockSize]);

    if (getxattr(p_filePath.c_str(), g_streamName.c_str(), (char*)(void*)buf.get(), blockSize) == 0) {
        const TTHStreamHeader& h = reinterpret_cast<const TTHStreamHeader&>(*buf.get());

        printf("%s: timestamps header=0x%llx, current=0x%llx, difference(should be zero)=%llx\n",
               p_filePath.c_str(),
               (long long unsigned int)h.timeStamp,
               (long long unsigned int)getTimeStamp(p_filePath),
               (long long unsigned int)(h.timeStamp - getTimeStamp(p_filePath)));

        if (!(h.timeStamp == getTimeStamp(p_filePath) && validateCheckSum(h))) { // File was modified and we should reset attr.
            deleteStream(p_filePath);

            return false;
        }

        const size_t datalen = blockSize - hdrSz;
        std::unique_ptr<uint8_t[]> tail(new uint8_t[datalen]);

        memcpy(tail.get(), (uint8_t*)buf.get() + hdrSz, datalen);

        TigerTree p_Tree = TigerTree(fileSize, h.blockSize, tail.get());

        if (p_Tree.getRoot() == h.root){
            tree = p_Tree;

            return true;
        }
        else
            return false;
    }
#else
    (void)p_filePath;
    (void)tree;
    (void)p_aFileSize;
#endif //USE_XATTR
    return false;
}

bool StreamStore::saveTree(const string& p_filePath, const TigerTree& p_Tree)
{
#ifdef USE_XATTR
    TTHStreamHeader h;

    h.fileSize = File::getSize(p_filePath);
    h.timeStamp = getTimeStamp(p_filePath);
    h.root = p_Tree.getRoot();
    h.blockSize = p_Tree.getBlockSize();

    setCheckSum(h);
    {
        const size_t sz = sizeof(TTHStreamHeader) + p_Tree.getLeaves().size() * TTHValue::BYTES;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[sz]);

        memcpy(buf.get(), &h, sizeof(TTHStreamHeader));
        memcpy(buf.get() + sizeof(TTHStreamHeader), p_Tree.getLeaves()[0].data, p_Tree.getLeaves().size() * TTHValue::BYTES);

        return (setxattr(p_filePath.c_str(), g_streamName.c_str(), (char*)(void*)buf.get(), sz, 0) == 0);
    }
#else // USE_XATTR
    (void)p_filePath;
    (void)p_Tree;
#endif // USE_XATTR
    return false;
}

void StreamStore::deleteStream(const string& p_filePath)
{
#ifdef USE_XATTR
    printf("Resetting Xattr for %s\n", p_filePath.c_str());
    removexattr(p_filePath.c_str(), g_streamName.c_str());
#else
    (void)p_filePath;
#endif //USE_XATTR
}


} // namespace dcpp
