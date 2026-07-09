/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2020 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ADC search send/receive and bloom GET handlers.
 */

#include "stdinc.h"
#include "AdcHub.h"

#include "SearchManager.h"
#include "SettingsManager.h"
#include "StringTokenizer.h"

namespace dcpp {

void AdcHub::handle(AdcCommand::SCH, AdcCommand& c) noexcept {
    OnlineUser* ou = findUser(c.getFrom());
    if(!ou) {
        dcdebug("Invalid user in AdcHub::onSCH\n");
        return;
    }

    fire(ClientListener::AdcSearch(), this, c, ou->getUser()->getCID());
}

void AdcHub::handle(AdcCommand::RES, AdcCommand& c) noexcept {
    OnlineUser* ou = findUser(c.getFrom());
    if(!ou) {
        dcdebug("Invalid user in AdcHub::onRES\n");
        return;
    }
    SearchManager::getInstance()->onRES(c, ou->getUser());
}

void AdcHub::handle(AdcCommand::PSR, AdcCommand& c) noexcept {
    OnlineUser* ou = findUser(c.getFrom());
    if(!ou) {
        dcdebug("Invalid user in AdcHub::onPSR\n");
        return;
    }
    SearchManager::getInstance()->onPSR(c, ou->getUser());
}

const vector<StringList>& AdcHub::getSearchExts() {
    if(!searchExts.empty())
        return searchExts;

    // the list is always immutable except for this function where it is initially being filled.
    const_cast<vector<StringList>&>(searchExts) = {
            // these extensions *must* be sorted alphabetically!
    { "ape", "flac", "m4a", "mid", "mp3", "mpc", "ogg", "ra", "wav", "wma" },
    { "7z", "ace", "arj", "bz2", "gz", "lha", "lzh", "rar", "tar", "z", "zip" },
    { "doc", "docx", "htm", "html", "nfo", "odf", "odp", "ods", "odt", "pdf", "ppt", "pptx", "rtf", "txt", "xls", "xlsx", "xml", "xps" },
    { "app", "bat", "cmd", "com", "dll", "exe", "jar", "msi", "ps1", "vbs", "wsf" },
    { "bmp", "cdr", "eps", "gif", "ico", "img", "jpeg", "jpg", "png", "ps", "psd", "sfw", "tga", "tif", "webp" },
    { "3gp", "asf", "asx", "avi", "divx", "flv", "mkv", "mov", "mp4", "mpeg", "mpg", "ogm", "pxp", "qt", "rm", "rmvb", "swf", "vob", "webm", "wmv" },
    { "iso", "mdf", "mds", "nrg", "vcd", "bwt", "ccd", "cdi", "pdi", "cue", "isz", "img", "vc4" }
};

    return searchExts;
}

StringList AdcHub::parseSearchExts(int flag) {
    StringList ret;
    const auto& searchExts = getSearchExts();
    for(auto i = searchExts.cbegin(), ibegin = i, iend = searchExts.cend(); i != iend; ++i) {
        if(flag & (1 << (i - ibegin))) {
            ret.insert(ret.begin(), i->begin(), i->end());
        }
    }
    return ret;
}

void AdcHub::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList) {
    if(state != STATE_NORMAL)
        return;

    AdcCommand c(AdcCommand::CMD_SCH, AdcCommand::TYPE_BROADCAST);

    if(!aToken.empty())
        c.addParam("TO", aToken);

    if(aFileType == SearchManager::TYPE_TTH) {
        c.addParam("TR", aString);
    } else {
        if(aSizeMode == SearchManager::SIZE_ATLEAST) {
            c.addParam("GE", Util::toString(aSize));
        } else if(aSizeMode == SearchManager::SIZE_ATMOST) {
            c.addParam("LE", Util::toString(aSize));
        }
        StringTokenizer<string> st(aString, ' ');
        for(auto& i: st.getTokens()) {
            c.addParam("AN", i);
        }
        if(aFileType == SearchManager::TYPE_DIRECTORY) {
            c.addParam("TY", "2");
        }

        if(aExtList.size() > 2) {
            StringList exts = aExtList;

            sort(exts.begin(), exts.end());

            uint8_t gr = 0;
            StringList rx;

            const auto& searchExts = getSearchExts();
            for(auto i = searchExts.cbegin(), ibegin = i, iend = searchExts.cend(); i != iend; ++i) {
                const StringList& def = *i;

                // gather the exts not present in any of the lists
                StringList temp(def.size() + exts.size());
                temp = StringList(temp.begin(), set_symmetric_difference(def.begin(), def.end(),
                                                                         exts.begin(), exts.end(), temp.begin()));

                // figure out whether the remaining exts have to be added or removed from the set
                StringList rx_;
                bool ok = true;
                for(auto diff = temp.begin(); diff != temp.end();) {
                    if(find(def.cbegin(), def.cend(), *diff) == def.cend()) {
                        ++diff; // will be added further below as an "EX"
                    } else {
                        if(rx_.size() == 2) {
                            ok = false;
                            break;
                        }
                        rx_.push_back(*diff);
                        diff = temp.erase(diff);
                    }
                }
                if(!ok) // too many "RX"s necessary - disregard this group
                    continue;

                // let's include this group!
                gr += 1 << (i - ibegin);

                exts = temp; // the exts to still add (that were not defined in the group)

                rx.insert(rx.begin(), rx_.begin(), rx_.end());

                if(exts.size() <= 2)
                    break;
                // keep looping to see if there are more exts that can be grouped
            }

            if(gr) {
                // some extensions can be grouped; let's send a command with grouped exts.
                AdcCommand c_gr(AdcCommand::CMD_SCH, AdcCommand::TYPE_FEATURE);
                c_gr.setFeatures('+' + SEGA_FEATURE);

                const auto& params = c.getParameters();
                for(auto& i: params)
                    c_gr.addParam(i);

                for(auto& i: exts)
                    c_gr.addParam("EX", i);
                c_gr.addParam("GR", Util::toString(gr));
                for(auto& i: rx)
                    c_gr.addParam("RX", i);

                sendSearch(c_gr);

                // make sure users with the feature don't receive the search twice.
                c.setType(AdcCommand::TYPE_FEATURE);
                c.setFeatures('-' + SEGA_FEATURE);
            }
        }

        for(auto& i: aExtList)
            c.addParam("EX", i);
    }

    sendSearch(c);
}

void AdcHub::sendSearch(AdcCommand& c) {
    if(isActive()) {
        send(c);
    } else {
        string features = c.getFeatures();
        c.setType(AdcCommand::TYPE_FEATURE);
        if (BOOLSETTING(ALLOW_NATT)) {
            c.setFeatures(features + '+' + TCP4_FEATURE + '-' + NAT0_FEATURE);
            send(c);
            c.setFeatures(features + '+' + NAT0_FEATURE);
        } else {
            c.setFeatures(features + '+' + TCP4_FEATURE);
        }
        send(c);
    }
}

} // namespace dcpp
