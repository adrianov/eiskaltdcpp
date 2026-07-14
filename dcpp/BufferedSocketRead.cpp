/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2019 EiskaltDC++ developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"

#include "BufferedSocket.h"

#include <algorithm>

#include "SettingsManager.h"
#include "ThrottleManager.h"
#include "ZUtils.h"
#include "format.h"

namespace dcpp {

using std::min;

void BufferedSocket::threadRead() {
    if(state != RUNNING || disconnecting)
        return;

    int left = (mode == MODE_DATA) ? ThrottleManager::getInstance()->read(sock.get(), &inbuf[0], (int)inbuf.size()) : sock->read(&inbuf[0], (int)inbuf.size());
    if(left == -1) {
        // EWOULDBLOCK, no data received...
        return;
    } else if(left == 0) {
        // This socket has been closed...
        throw SocketException(_("Connection closed"));
    }
    string::size_type pos = 0;
    // always uncompressed data
    string l;
    int bufpos = 0, total = left;

    while (left > 0) {
        if(disconnecting)
            return;
        switch (mode) {
        case MODE_ZPIPE: {
            const int BUF_SIZE = 1024;
            // Special to autodetect nmdc connections...
            string::size_type pos = 0;
            std::unique_ptr<char[]> buffer(new char[BUF_SIZE]);
            l = line;
            // decompress all input data and store in l.
            while (left) {
                if(disconnecting)
                    return;
                size_t in = BUF_SIZE;
                size_t used = left;
                bool ret = (*filterIn) (&inbuf[0] + total - left, used, &buffer[0], in);
                if(used == 0)
                    break;
                left -= used;
                l.append (&buffer[0], in);
                // if the stream ends before the data runs out, keep remainder of data in inbuf
                if (!ret) {
                    bufpos = total-left;
                    setMode (MODE_LINE, rollback);
                    break;
                }
            }
            // process all lines
            while ((pos = l.find(separator)) != string::npos) {
                if(disconnecting)
                    return;
                if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
                    fire(BufferedSocketListener::Line(), l.substr(0, pos));
                l.erase (0, pos + 1 /* separator char */);
            }
            // store remainder
            line = l;

            break;
        }
        case MODE_LINE:
            // Special to autodetect nmdc connections...
            if(separator == 0) {
                if(inbuf[0] == '$') {
                    separator = '|';
                } else {
                    separator = '\n';
                }
            }
            l = line + string ((char*)&inbuf[bufpos], left);
            while ((pos = l.find(separator)) != string::npos) {
                if(disconnecting)
                    return;
                if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
                    fire(BufferedSocketListener::Line(), l.substr(0, pos));
                l.erase (0, pos + 1 /* separator char */);
                if (l.length() < (size_t)left) left = l.length();
                if (mode != MODE_LINE) {
                    // we changed mode; remainder of l is invalid.
                    l.clear();
                    bufpos = total - left;
                    break;
                }
            }
            if (pos == string::npos)
                left = 0;
            line = l;
            break;
        case MODE_DATA:
            while(left > 0) {
                if(disconnecting)
                    return;
                if(dataBytes == -1) {
                    fire(BufferedSocketListener::Data(), &inbuf[bufpos], left);
                    bufpos += (left - rollback);
                    left = rollback;
                    rollback = 0;
                } else {
                    int high = (int)min(dataBytes, (int64_t)left);
                    fire(BufferedSocketListener::Data(), &inbuf[bufpos], high);
                    bufpos += high;
                    left -= high;

                    dataBytes -= high;
                    if(dataBytes == 0) {
                        mode = MODE_LINE;
                        fire(BufferedSocketListener::ModeChange());
                    }
                }
            }
            break;
        }
    }

    if(mode == MODE_LINE && line.size() > static_cast<size_t>(SETTING(MAX_COMMAND_LENGTH))) {
        throw SocketException(_("Maximum command length exceeded"));
    }
}

} // namespace dcpp
