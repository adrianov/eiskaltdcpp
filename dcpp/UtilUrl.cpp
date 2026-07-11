/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2026 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/** URL helpers: sanitize/trim, decode, query parse, URI encode, hub scheme checks. */

#include "stdinc.h"

#include "Util.h"

#include <array>
#include <cctype>
#include <map>

#ifdef USE_IDN2
#include <idn2.h>
#endif

namespace dcpp {

void Util::sanitizeUrl(string& url) {
    if(url.empty())
        return;
    static const std::array<char, 7> special_chars = { ' ', '<', '>', '"', '\t', '\r', '\n' };
    for(const auto &ch : special_chars) {
        while(!url.empty() && url.front() == ch)
            url.erase(0, 1);
        while(!url.empty() && url.back() == ch)
            url.erase(url.length() - 1);
    }
}

string Util::trimCopy(const string &aLine) {
    string out = aLine;
    sanitizeUrl(out);
    return out;
}

/** Decode URL; default ports: http 80, https 443, dchub 411. */
void Util::decodeUrl(const string& url, string& protocol, string& host, string& port, string& path, string& query, string& fragment) {
    auto fragmentEnd = url.size();
    auto fragmentStart = url.rfind('#');

    size_t queryEnd;
    if(fragmentStart == string::npos) {
        queryEnd = fragmentStart = fragmentEnd;
    } else {
        queryEnd = fragmentStart;
        fragmentStart++;
    }

    auto queryStart = url.rfind('?', queryEnd);
    size_t fileEnd;

    if(queryStart == string::npos) {
        fileEnd = queryStart = queryEnd;
    } else {
        fileEnd = queryStart;
        queryStart++;
    }

    auto protoStart = 0;
    auto protoEnd = url.find("://", protoStart);

    auto authorityStart = protoEnd == string::npos ? protoStart : protoEnd + 3;
    auto authorityEnd = url.find_first_of("/#?", authorityStart);

    size_t fileStart;
    if(authorityEnd == string::npos) {
        authorityEnd = fileStart = fileEnd;
    } else {
        fileStart = authorityEnd;
    }

    protocol = (protoEnd == string::npos ? Util::emptyString : url.substr(protoStart, protoEnd - protoStart));

    if(authorityEnd > authorityStart) {
        size_t portStart = string::npos;
        if(url[authorityStart] == '[') {
            auto hostEnd = url.find(']');
            if(hostEnd == string::npos) {
                return;
            }

            host = url.substr(authorityStart + 1, hostEnd - authorityStart - 1);
            if(hostEnd + 1 < url.size() && url[hostEnd + 1] == ':') {
                portStart = hostEnd + 2;
            }
        } else {
            size_t hostEnd;
            portStart = url.find(':', authorityStart);
            if(portStart != string::npos && portStart > authorityEnd) {
                portStart = string::npos;
            }

            if(portStart == string::npos) {
                hostEnd = authorityEnd;
            } else {
                hostEnd = portStart;
                portStart++;
            }

            host = url.substr(authorityStart, hostEnd - authorityStart);
        }

        if(portStart == string::npos) {
            if(protocol == "http") {
                port = "80";
            } else if(protocol == "https") {
                port = "443";
            } else if(protocol == "dchub"  || protocol.empty()) {
                port = "411";
            }
        } else {
            port = url.substr(portStart, authorityEnd - portStart);
        }
    }

    path = url.substr(fileStart, fileEnd - fileStart);
    query = url.substr(queryStart, queryEnd - queryStart);
    fragment = url.substr(fragmentStart, fragmentEnd - fragmentStart);

#ifdef USE_IDN2
    char *p;
    if (idn2_to_ascii_8z(host.c_str(), &p, IDN2_NONTRANSITIONAL) == IDN2_OK) {
        host = string(p);
    }
    free(p);
#endif
}

map<string, string> Util::decodeQuery(const string& query) {
    map<string, string> ret;
    size_t start = 0;
    while(start < query.size()) {
        auto eq = query.find('=', start);
        if(eq == string::npos) {
            break;
        }

        auto param = eq + 1;
        auto end = query.find('&', param);

        if(end == string::npos) {
            end = query.size();
        }

        if(eq > start && end > param) {
            ret[query.substr(start, eq-start)] = query.substr(param, end - param);
        }

        start = end + 1;
    }

    return ret;
}

string Util::encodeURI(const string& aString, bool reverse) {
    // reference: rfc2396
    string tmp = aString;
    if(reverse) {
        string::size_type idx;
        for(idx = 0; idx < tmp.length(); ++idx) {
            if(tmp.length() > idx + 2 && tmp[idx] == '%' && isxdigit(tmp[idx+1]) && isxdigit(tmp[idx+2])) {
                tmp[idx] = fromHexEscape(tmp.substr(idx+1,2));
                tmp.erase(idx+1, 2);
            } else { // reference: rfc1630, magnet-uri draft
                if(tmp[idx] == '+')
                    tmp[idx] = ' ';
            }
        }
    } else {
        const string disallowed = ";/?:@&=+$," // reserved
                "<>#%\" "    // delimiters
                "{}|\\^[]`"; // unwise
        string::size_type idx, loc;
        for(idx = 0; idx < tmp.length(); ++idx) {
            if(tmp[idx] == ' ') {
                tmp[idx] = '+';
            } else {
                if(tmp[idx] <= 0x1F || tmp[idx] >= 0x7f || (loc = disallowed.find_first_of(tmp[idx])) != string::npos) {
                    tmp.replace(idx, 1, toHexEscape(tmp[idx]));
                    idx+=2;
                }
            }
        }
    }
    return tmp;
}

} // namespace dcpp
