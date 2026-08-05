/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * NMDC hub extensions shared with FlylinkDC++: HubURL, NickRule, SearchRule, BadNick.
 */

#include "stdinc.h"

#include "NmdcHub.h"

#include "FavoriteManager.h"
#include "HubReconnectFilter.h"
#include "StringTokenizer.h"
#include "Util.h"
#include "format.h"

namespace dcpp {

void NmdcHub::fillNickField(NickRule& rule, const string& key, const string& val) {
    if(key == "Min" || key == "TooShort") {
        unsigned n = Util::toInt(val);
        rule.minLen = n > 64 ? 64 : n;
    } else if(key == "Max" || key == "TooLong") {
        unsigned n = Util::toInt(val);
        rule.maxLen = n > 200 ? 200 : n;
    } else if(key == "Char" || key == "BadChar") {
        StringTokenizer<string> chars(val, ' ');
        for(const auto& c: chars.getTokens()) {
            if(!c.empty())
                rule.badChars.push_back(static_cast<char>(Util::toInt(c)));
        }
    } else if((key == "Pref" || key == "BadPrefix") && !val.empty()) {
        StringTokenizer<string> prefs(val, ' ');
        for(const auto& p: prefs.getTokens()) {
            if(!p.empty())
                rule.prefixes.push_back(p);
        }
    }
}

void NmdcHub::parseNickFields(NickRule& rule, const string& param) {
    StringTokenizer<string> st(param, "$$");
    for(const auto& part: st.getTokens()) {
        const auto sp = part.find(' ');
        if(sp == string::npos)
            fillNickField(rule, part, Util::emptyString);
        else
            fillNickField(rule, part.substr(0, sp), part.substr(sp + 1));
    }
}

void NmdcHub::NickRule::convert(string& nick) const {
    for(char c: badChars)
        std::replace(nick.begin(), nick.end(), c, '_');
    if(!prefixes.empty()) {
        bool hasPref = false;
        for(const auto& p: prefixes) {
            if(nick.compare(0, p.size(), p) == 0) {
                hasPref = true;
                break;
            }
        }
        if(!hasPref)
            nick = prefixes[0] + nick;
    }
    if(maxLen && nick.length() > maxLen)
        nick.resize(maxLen);
    const unsigned floor = (maxLen && minLen > maxLen) ? maxLen : minLen;
    if(floor && nick.length() < floor) {
        nick += "_R";
        while(nick.length() < floor) {
            const size_t need = floor - nick.length();
            nick += Util::toString(Util::rand()).substr(0, need);
        }
    }
}

void NmdcHub::raiseSearchFloor(int seconds) {
    if(seconds < 0) {
        setSearchBlocked(true);
        return;
    }
    if(seconds <= 0)
        return;
    const uint32_t sec = static_cast<uint32_t>(seconds);
    const uint64_t ms = (uint64_t)(sec + min(sec, (uint32_t)1)) * 1000;
    if(searchQueue.interval < ms)
        searchQueue.interval = ms;
}

bool NmdcHub::passiveSearch() const {
    return !isActive() || BOOLSETTING(SEARCH_PASSIVE);
}

void NmdcHub::reconnectForNick() {
    if(sock)
        disconnect(true);
    setAutoReconnect(true);
    HubReconnectFilter::clearToday(getHubUrl());
    setReconnAttempts(0);
    setReconnDelay(0);
    updateActivity();
}

bool NmdcHub::applyNickRule(bool reconnectIfChanged) {
    // Registered accounts keep a fixed nick (NMDC NickRule / BadNick).
    if(!nickRule || getMyIdentity().isSet("RG"))
        return false;
    const string oldNick = getCurrentNick();
    string nick = oldNick;
    nickRule->convert(nick);
    nick = checkNick(nick);
    if(nick == oldNick)
        return false;

    FavoriteManager::getInstance()->setHubNick(getHubUrl(), nick);
    setCurrentNick(nick);
    fire(ClientListener::StatusMessage(), this,
         str(F_("Nick \"%1%\" rejected by hub rules; switching to \"%2%\"...") % oldNick % nick),
         ClientListener::FLAG_NORMAL);

    if(reconnectIfChanged && state != STATE_PROTOCOL)
        reconnectForNick();
    return true;
}

void NmdcHub::onLineHubExt(const string& cmd, const string& param) {
    if(cmd == "$HubTopic") {
        fire(ClientListener::StatusMessage(), this,
             _("Hub topic:") + string(" ") + unescape(param), ClientListener::FLAG_NORMAL);
        return;
    }
    if(cmd == "$GetHubURL") {
        send("$MyHubURL " + getHubUrl() + "|");
        return;
    }
    if(cmd == "$SearchRule") {
        bool haveInt = false, haveIntPas = false;
        int activeSec = 0, passiveSec = 0;
        StringTokenizer<string> st(param, "$$");
        for(const auto& part: st.getTokens()) {
            const auto sp = part.find(' ');
            if(sp == string::npos)
                continue;
            const string key = part.substr(0, sp);
            const int sec = Util::toInt(part.substr(sp + 1));
            if(key == "Int") {
                haveInt = true;
                activeSec = sec;
            } else if(key == "IntPas") {
                haveIntPas = true;
                passiveSec = sec;
            }
        }
        if(passiveSearch()) {
            if(haveIntPas)
                raiseSearchFloor(passiveSec);
        } else if(haveInt) {
            raiseSearchFloor(activeSec);
        }
        return;
    }
    if(cmd == "$NickRule") {
        nickRule = make_unique<NickRule>();
        parseNickFields(*nickRule, param);
        applyNickRule(true);
        return;
    }
    if(cmd == "$BadNick") {
        if(!nickRule && !param.empty()) {
            nickRule = make_unique<NickRule>();
            parseNickFields(*nickRule, param);
        }
        if(applyNickRule(false)) {
            reconnectForNick();
            return;
        }
        disconnect(false);
        fire(ClientListener::NickTaken(), this);
    }
}

} // namespace dcpp
