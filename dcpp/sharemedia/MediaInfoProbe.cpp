/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 * Adapted from FlylinkDC++ getMediaInfo (flyServer.cpp).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "MediaInfoProbe.h"
#include "Text.h"
#include "Util.h"
#include "CriticalSection.h"

#ifdef USE_MEDIAINFO
#include <MediaInfo/MediaInfo.h>
#include <unordered_map>
#endif

namespace dcpp {

#ifdef USE_MEDIAINFO

namespace {

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

string miGet(MediaInfoLib::MediaInfo& lib, MediaInfoLib::stream_t stream,
             size_t i, const wchar_t* key)
{
    return Text::fromT(wstring(lib.Get(stream, i, key)));
}

void appendField(string& s, const string& v)
{
    if (v.empty())
        return;
    s += ' ';
    s += v;
    s += ',';
}

string trimComma(string s)
{
    if (!s.empty() && s.back() == ',')
        s.pop_back();
    return s;
}

void readAudio(MediaInfoLib::MediaInfo& lib, MediaInfo& out)
{
    unordered_map<string, uint16_t> audioDup;
    const size_t n = lib.Count_Get(MediaInfoLib::Stream_Audio);
    for (size_t i = 0; i < n; ++i) {
        const uint16_t bitRate = static_cast<uint16_t>(
                Util::toFloat(miGet(lib, MediaInfoLib::Stream_Audio, i, L"BitRate")) / 1000.0 + 0.5);
        if (bitRate > out.bitrate)
            out.bitrate = bitRate;

        wstring sFormat = lib.Get(MediaInfoLib::Stream_Audio, i, L"Format");
        replaceAll(sFormat, L" Audio", wstring());
        const string sBitRate = miGet(lib, MediaInfoLib::Stream_Audio, i, L"BitRate/String");
        const wstring sChannelPos = lib.Get(MediaInfoLib::Stream_Audio, i, L"ChannelPositions");
        const uint16_t iChannels = static_cast<uint16_t>(
                Util::toInt(miGet(lib, MediaInfoLib::Stream_Audio, i, L"Channel(s)")));
        const string sChannels = sChannelPos.find(L"LFE") != wstring::npos
                ? Util::toString(iChannels > 0 ? iChannels - 1 : 0) + ".1"
                : Util::toString(iChannels) + ".0";
        const string sLanguage = miGet(lib, MediaInfoLib::Stream_Audio, i, L"Language/String1");

        string row;
        appendField(row, Text::fromT(sFormat));
        appendField(row, sChannels);
        appendField(row, sBitRate);
        appendField(row, sLanguage);
        row = trimComma(row);
        if (!row.empty())
            audioDup[row]++;
    }

    string audioAll;
    string sep;
    for (const auto& k : audioDup) {
        audioAll += sep;
        audioAll += k.second == 1 ? k.first : k.first + " (x" + Util::toString(k.second) + ")";
        sep = " |";
    }

    const string duration = miGet(lib, MediaInfoLib::Stream_General, 0, L"Duration/String");
    if (!duration.empty() || !audioAll.empty()) {
        if (!duration.empty())
            out.audio_info = duration + " |";
        out.audio_info += audioAll;
    }
}

void fillResolution(MediaInfoLib::MediaInfo& lib, MediaInfoLib::stream_t stream, MediaInfo& out)
{
    if (!out.resolution.empty())
        return;
    const int mediaX = Util::toInt(miGet(lib, stream, 0, L"Width"));
    const int mediaY = mediaX ? Util::toInt(miGet(lib, stream, 0, L"Height")) : 0;
    if (mediaX && mediaY)
        out.resolution = Util::toString(mediaX) + "x" + Util::toString(mediaY);
}

void readVideo(MediaInfoLib::MediaInfo& lib, MediaInfo& out)
{
    fillResolution(lib, MediaInfoLib::Stream_Video, out);
    fillResolution(lib, MediaInfoLib::Stream_Image, out);

    const size_t n = lib.Count_Get(MediaInfoLib::Stream_Video);
    if (n == 0)
        return;

    string videoString;
    for (size_t i = 0; i < n; ++i) {
        wstring sVFormat = lib.Get(MediaInfoLib::Stream_Video, i, L"Format");
        replaceAll(sVFormat, L" Video", wstring());
        replaceAll(sVFormat, L" Visual", wstring());
        const string sVBitrate = miGet(lib, MediaInfoLib::Stream_Video, i, L"BitRate/String");
        const string sVFrameRate = miGet(lib, MediaInfoLib::Stream_Video, i, L"FrameRate/String");
        if (sVFormat.empty() && sVBitrate.empty() && sVFrameRate.empty())
            continue;
        string row;
        if (!sVFormat.empty())
            row += Text::fromT(sVFormat) + ", ";
        if (!sVBitrate.empty())
            row += sVBitrate + ", ";
        if (!sVFrameRate.empty())
            row += sVFrameRate + ", ";
        if (row.size() >= 2)
            row.erase(row.size() - 2);
        if (!row.empty()) {
            videoString += row;
            videoString += " | ";
        }
    }
    if (videoString.size() > 3)
        out.video_info = videoString.substr(0, videoString.size() - 3);
}

} // namespace

bool MediaInfoProbe::scan(const string& path, MediaInfo& out)
{
    out = MediaInfo();
    static CriticalSection scanCs;
    Lock lock(scanCs);
    static MediaInfoLib::MediaInfo lib;

    if (!lib.Open(Text::toT(path)))
        return false;
    readAudio(lib, out);
    readVideo(lib, out);
    lib.Close();
    return !out.empty();
}

#else

bool MediaInfoProbe::scan(const string& path, MediaInfo& out)
{
    (void)path;
    out = MediaInfo();
    return false;
}

#endif

} // namespace dcpp
