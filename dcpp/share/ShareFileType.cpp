/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "share/ShareFileType.h"

#include "Text.h"
#include "Util.h"

namespace dcpp {

namespace {

// Short (3-char) and long extension sets match the historical ShareManager::checkType
// tables, including the partial long-ext slices that code always used.
const char* const audioShort[] = {
    ".mp3", ".mp2", ".mid", ".wav", ".ogg", ".wma", ".669", ".aac", ".aif", ".amf", ".ams", ".ape",
    ".dbm", ".dmf", ".dsm", ".far", ".mdl", ".med", ".mod", ".mol", ".mp1", ".mpa", ".mpc", ".mpp",
    ".mtm", ".nst", ".okt", ".psm", ".ptm", ".rmi", ".s3m", ".stm", ".ult", ".umx", ".wow"
};
const char* const compressedShort[] = { ".rar", ".zip", ".ace" };
const char* const documentShort[] = { ".htm", ".doc", ".txt", ".nfo" };
const char* const executableShort[] = { ".exe" };
const char* const pictureShort[] = {
    ".jpg", ".gif", ".png", ".eps", ".img", ".pct", ".psp", ".pic", ".tif", ".rle", ".bmp", ".pcx",
    ".jpe", ".dcx", ".emf", ".ico", ".psd", ".tga", ".wmf", ".xif"
};
const char* const videoShort[] = {
    ".avi", ".mpg", ".mov", ".flv", ".asf", ".pxp", ".wmv", ".ogm", ".mkv", ".m1v", ".m2v", ".mpe",
    ".mps", ".mpv", ".ram", ".vob", ".mp4"
};
const char* const cdImageShort[] = {
    ".iso", ".mdf", ".mds", ".nrg", ".vcd", ".bwt", ".ccd", ".cdi", ".pdi", ".cue", ".isz", ".img", ".vc4"
};

const string audioLong[] = { ".au", ".it", ".ra" };
const string pictureLong[] = { ".ai", ".ps", ".pict" };
const string videoLong[] = { ".rm", ".divx", ".mpeg" };

template<size_t N>
constexpr size_t countOf(const char* const (&)[N]) { return N; }
template<size_t N>
constexpr size_t countOf(const string (&)[N]) { return N; }

} // namespace

const ShareFileType::Bucket* ShareFileType::buckets(size_t& count) {
    static const Bucket table[] = {
        { SearchManager::TYPE_AUDIO, audioShort, countOf(audioShort), audioLong, countOf(audioLong) },
        { SearchManager::TYPE_CD_IMAGE, cdImageShort, countOf(cdImageShort), nullptr, 0 },
        { SearchManager::TYPE_COMPRESSED, compressedShort, countOf(compressedShort), nullptr, 0 },
        { SearchManager::TYPE_DOCUMENT, documentShort, countOf(documentShort), nullptr, 0 },
        { SearchManager::TYPE_EXECUTABLE, executableShort, countOf(executableShort), nullptr, 0 },
        { SearchManager::TYPE_PICTURE, pictureShort, countOf(pictureShort), pictureLong, countOf(pictureLong) },
        { SearchManager::TYPE_VIDEO, videoShort, countOf(videoShort), videoLong, countOf(videoLong) },
    };
    count = sizeof(table) / sizeof(table[0]);
    return table;
}

uint32_t ShareFileType::packExt3(const string& fileName) {
    const char* c = fileName.c_str() + fileName.length() - 3;
    return '.' | (Text::asciiToLower(c[0]) << 8) | (Text::asciiToLower(c[1]) << 16)
            | (static_cast<uint32_t>(Text::asciiToLower(c[2])) << 24);
}

bool ShareFileType::matchShort(uint32_t packed, const char* const* exts, size_t count) {
    for(size_t i = 0; i < count; ++i) {
        if(packed == *reinterpret_cast<const uint32_t*>(exts[i]))
            return true;
    }
    return false;
}

bool ShareFileType::matchLong(const string& fileName, const string* exts, size_t count) {
    for(size_t i = 0; i < count; ++i) {
        const string& ext = exts[i];
        if(fileName.length() >= ext.length()
                && Util::stricmp(fileName.c_str() + fileName.length() - ext.length(), ext.c_str()) == 0)
            return true;
    }
    return false;
}

bool ShareFileType::matchBucket(const string& fileName, uint32_t packed, const Bucket& bucket) {
    return matchShort(packed, bucket.shortExts, bucket.shortCount)
            || (bucket.longCount && matchLong(fileName, bucket.longExts, bucket.longCount));
}

bool ShareFileType::matches(const string& fileName, int type) {
    if(type == SearchManager::TYPE_ANY)
        return true;
    if(type == SearchManager::TYPE_AUDIO_VIDEO)
        return matches(fileName, SearchManager::TYPE_AUDIO) || matches(fileName, SearchManager::TYPE_VIDEO);
    if(fileName.length() < 5)
        return false;

    const char* c = fileName.c_str() + fileName.length() - 3;
    if(!Text::isAscii(c))
        return false;

    const uint32_t packed = packExt3(fileName);
    size_t n = 0;
    const Bucket* table = buckets(n);
    for(size_t i = 0; i < n; ++i) {
        if(table[i].type == type)
            return matchBucket(fileName, packed, table[i]);
    }
    return false;
}

SearchManager::TypeModes ShareFileType::classify(const string& fileName) noexcept {
    if(!fileName.empty() && fileName[fileName.length() - 1] == PATH_SEPARATOR)
        return SearchManager::TYPE_DIRECTORY;

    static const int order[] = {
        SearchManager::TYPE_VIDEO,
        SearchManager::TYPE_AUDIO,
        SearchManager::TYPE_COMPRESSED,
        SearchManager::TYPE_DOCUMENT,
        SearchManager::TYPE_EXECUTABLE,
        SearchManager::TYPE_PICTURE,
        SearchManager::TYPE_CD_IMAGE
    };
    for(int type : order) {
        if(matches(fileName, type))
            return static_cast<SearchManager::TypeModes>(type);
    }
    return SearchManager::TYPE_ANY;
}

} // namespace dcpp
