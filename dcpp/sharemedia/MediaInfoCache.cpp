/*
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 * Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "stdinc.h"
#include "MediaInfoCache.h"
#include "File.h"
#include "SimpleXML.h"
#include "Util.h"

namespace dcpp {

namespace {

const string& tagRoot() { static string s = "MediaInfoCache"; return s; }
const string& tagFile() { static string s = "File"; return s; }

} // namespace

string MediaInfoCache::cachePath()
{
    return Util::getPath(Util::PATH_USER_CONFIG) + "MediaInfo.xml";
}

void MediaInfoCache::load()
{
    if (loaded)
        return;
    loaded = true;
    try {
        SimpleXML xml;
        xml.fromXML(File(cachePath(), File::READ, File::OPEN).read());
        xml.resetCurrentChild();
        if (!xml.findChild(tagRoot()))
            return;
        xml.stepIn();
        while (xml.findChild(tagFile())) {
            const string tth = xml.getChildAttrib("TTH");
            if (tth.size() != 39)
                continue;
            MediaInfo m;
            m.bitrate = static_cast<uint16_t>(Util::toInt(xml.getChildAttrib("BR")));
            m.resolution = xml.getChildAttrib("WH");
            m.video_info = xml.getChildAttrib("MV");
            m.audio_info = xml.getChildAttrib("MA");
            map[TTHValue(tth)] = m;
        }
        xml.stepOut();
    } catch (const Exception&) {
        // First run or corrupt cache.
    }
}

void MediaInfoCache::save()
{
    Lock l(cs);
    if (!dirty)
        return;
    try {
        SimpleXML xml;
        xml.addTag(tagRoot());
        xml.stepIn();
        for (const auto& i : map) {
            xml.addTag(tagFile());
            xml.addChildAttrib("TTH", i.first.toBase32());
            if (i.second.bitrate)
                xml.addChildAttrib("BR", Util::toString(i.second.bitrate));
            if (!i.second.resolution.empty())
                xml.addChildAttrib("WH", i.second.resolution);
            if (!i.second.video_info.empty())
                xml.addChildAttrib("MV", i.second.video_info);
            if (!i.second.audio_info.empty())
                xml.addChildAttrib("MA", i.second.audio_info);
            // Empty entry marks "scanned, no media" (no attribs besides TTH).
        }
        xml.stepOut();
        const string path = cachePath();
        const string tmp = path + ".tmp";
        File f(tmp, File::WRITE, File::CREATE | File::TRUNCATE);
        f.write(SimpleXML::utf8Header);
        f.write(xml.toXML());
        f.close();
        File::renameFile(tmp, path);
        dirty = false;
    } catch (const Exception&) {
    }
}

bool MediaInfoCache::get(const TTHValue& tth, MediaInfo& out)
{
    Lock l(cs);
    load();
    auto i = map.find(tth);
    if (i == map.end())
        return false;
    out = i->second;
    return true;
}

void MediaInfoCache::put(const TTHValue& tth, const MediaInfo& info)
{
    Lock l(cs);
    load();
    map[tth] = info;
    dirty = true;
}

} // namespace dcpp
