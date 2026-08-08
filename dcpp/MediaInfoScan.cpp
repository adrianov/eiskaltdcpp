/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Adapted from FlylinkDC++ getMediaInfo (flyServer.cpp).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "MediaInfoScan.h"
#include "MediaInfoCache.h"
#include "SettingsManager.h"
#include "Text.h"
#include "Util.h"
#include "File.h"
#include "LogManager.h"
#include "CriticalSection.h"

#ifdef USE_MEDIAINFO
#include <MediaInfo/MediaInfo.h>
#include <unordered_map>
#endif

namespace dcpp {

namespace {

// Flylink compiled/Settings mediainfo_ext (+ mxf).
const char* mediaExts[] = {
    "3gp", "avi", "divx", "flv", "m4v", "mkv", "dts", "mov", "mp4", "mpg", "mpeg",
    "vob", "wmv", "bik", "qt", "rm", "aac", "ac3", "ape", "fla", "flac", "m4a",
    "mp1", "mp2", "mp3", "ogg", "wma", "wv", "mka", "vqf", "lqt", "ts", "tp",
    "mod", "m2ts", "webm", "mpe", "mxf", nullptr
};

string extWithoutDot(const string& path)
{
    string e = Util::getFileExt(path);
    if (!e.empty() && e[0] == '.')
        e.erase(0, 1);
    return Text::toLower(e);
}

void replaceAll(wstring& s, const wstring& from, const wstring& to)
{
    if (from.empty())
        return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

#ifdef USE_MEDIAINFO
string miGet(MediaInfoLib::MediaInfo& lib, MediaInfoLib::stream_t stream, size_t i, const wchar_t* key)
{
    return Text::fromT(wstring(lib.Get(stream, i, key)));
}
#endif

} // namespace

bool isMediaInfoExt(const string& extWithoutDot)
{
    const string e = Text::toLower(extWithoutDot);
    for (const char** p = mediaExts; *p; ++p) {
        if (e == *p)
            return true;
    }
    return false;
}

bool mediaInfoFill(const string& path, int64_t size, const TTHValue& tth, MediaInfo& out)
{
    out = MediaInfo();
    if (MediaInfoCache::getInstance()->get(tth, out))
        return out.bitrate != 0 || !out.resolution.empty()
                || !out.video_info.empty() || !out.audio_info.empty();

#ifndef USE_MEDIAINFO
    MediaInfoCache::getInstance()->put(tth, out);
    return false;
#else
    if (size < 0)
        size = File::getSize(path);
    if (size < int64_t(SETTING(MIN_MEDIAINFO_SIZE)) * 1024 * 1024)
        return false;
    if (!isMediaInfoExt(extWithoutDot(path)))
        return false;

    try {
        static CriticalSection scanCs;
        Lock lock(scanCs);
        static MediaInfoLib::MediaInfo lib;

        if (!lib.Open(Text::toT(path))) {
            MediaInfoCache::getInstance()->put(tth, out);
            return false;
        }

        const size_t audioCount = lib.Count_Get(MediaInfoLib::Stream_Audio);
        out.bitrate = 0;
        unordered_map<string, uint16_t> audioDup;
        for (size_t i = 0; i < audioCount; ++i) {
            const string sinfo = miGet(lib, MediaInfoLib::Stream_Audio, i, L"BitRate");
            const uint16_t bitRate = static_cast<uint16_t>(Util::toFloat(sinfo) / 1000.0 + 0.5);
            if (bitRate > out.bitrate)
                out.bitrate = bitRate;

            wstring sFormat = lib.Get(MediaInfoLib::Stream_Audio, i, L"Format");
            replaceAll(sFormat, L" Audio", wstring());
            const string sBitRate = miGet(lib, MediaInfoLib::Stream_Audio, i, L"BitRate/String");
            const wstring sChannelPos = lib.Get(MediaInfoLib::Stream_Audio, i, L"ChannelPositions");
            const uint16_t iChannels = static_cast<uint16_t>(
                    Util::toInt(miGet(lib, MediaInfoLib::Stream_Audio, i, L"Channel(s)")));
            string sChannels;
            if (sChannelPos.find(L"LFE") != wstring::npos)
                sChannels = Util::toString(iChannels > 0 ? iChannels - 1 : 0) + ".1";
            else
                sChannels = Util::toString(iChannels) + ".0";
            const string sLanguage = miGet(lib, MediaInfoLib::Stream_Audio, i, L"Language/String1");

            string audioFormatString;
            if (!sFormat.empty() || !sBitRate.empty() || !sChannels.empty() || !sLanguage.empty()) {
                if (!sFormat.empty()) {
                    audioFormatString += ' ';
                    audioFormatString += Text::fromT(sFormat);
                    audioFormatString += ',';
                }
                if (!sChannels.empty()) {
                    audioFormatString += ' ';
                    audioFormatString += sChannels;
                    audioFormatString += ',';
                }
                if (!sBitRate.empty()) {
                    audioFormatString += ' ';
                    audioFormatString += sBitRate;
                    audioFormatString += ',';
                }
                if (!sLanguage.empty()) {
                    audioFormatString += ' ';
                    audioFormatString += sLanguage;
                    audioFormatString += ',';
                }
                if (!audioFormatString.empty() && audioFormatString.back() == ',')
                    audioFormatString.pop_back();
                audioDup[audioFormatString]++;
            }
        }

        string audioAll;
        string sep;
        for (const auto& k : audioDup) {
            audioAll += sep;
            if (k.second == 1)
                audioAll += k.first;
            else
                audioAll += k.first + " (x" + Util::toString(k.second) + ")";
            sep = " |";
        }

        const string width = miGet(lib, MediaInfoLib::Stream_Video, 0, L"Width");
        string height;
        if (!width.empty())
            height = miGet(lib, MediaInfoLib::Stream_Video, 0, L"Height");
        const int mediaX = Util::toInt(width);
        const int mediaY = mediaX ? Util::toInt(height) : 0;
        if (mediaX && mediaY)
            out.resolution = Util::toString(mediaX) + "x" + Util::toString(mediaY);

        const string sDuration = miGet(lib, MediaInfoLib::Stream_General, 0, L"Duration/String");
        if (!sDuration.empty() || !audioAll.empty()) {
            if (!sDuration.empty())
                out.audio_info = sDuration + " |";
            if (!audioAll.empty())
                out.audio_info += audioAll;
        }

        const size_t videoCount = lib.Count_Get(MediaInfoLib::Stream_Video);
        if (videoCount > 0) {
            string videoString;
            for (size_t i = 0; i < videoCount; ++i) {
                wstring sVFormat = lib.Get(MediaInfoLib::Stream_Video, i, L"Format");
                replaceAll(sVFormat, L" Video", wstring());
                replaceAll(sVFormat, L" Visual", wstring());
                const string sVBitrate = miGet(lib, MediaInfoLib::Stream_Video, i, L"BitRate/String");
                const string sVFrameRate = miGet(lib, MediaInfoLib::Stream_Video, i, L"FrameRate/String");
                if (!sVFormat.empty() || !sVBitrate.empty() || !sVFrameRate.empty()) {
                    if (!sVFormat.empty()) {
                        videoString += Text::fromT(sVFormat);
                        videoString += ", ";
                    }
                    if (!sVBitrate.empty()) {
                        videoString += sVBitrate;
                        videoString += ", ";
                    }
                    if (!sVFrameRate.empty()) {
                        videoString += sVFrameRate;
                        videoString += ", ";
                    }
                    if (videoString.size() >= 2)
                        videoString.erase(videoString.size() - 2);
                    videoString += " | ";
                }
            }
            if (videoString.size() > 3)
                out.video_info = videoString.substr(0, videoString.size() - 3);
        }

        lib.Close();
        MediaInfoCache::getInstance()->put(tth, out);
        return out.bitrate != 0 || !out.resolution.empty()
                || !out.video_info.empty() || !out.audio_info.empty();
    } catch (const std::exception& e) {
        LogManager::getInstance()->message(string("MediaInfo: ") + path + " (" + e.what() + ")");
        MediaInfoCache::getInstance()->put(tth, out);
        return false;
    } catch (...) {
        MediaInfoCache::getInstance()->put(tth, out);
        return false;
    }
#endif
}

} // namespace dcpp
