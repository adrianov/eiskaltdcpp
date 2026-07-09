/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2009-2026 EiskaltDC++ developers
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

#include "SettingsManager.h"

#include "Util.h"
#include "format.h"

namespace dcpp {

void SettingsManager::setDefaults() {
    setDefault(DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_DOWNLOADS));
    setDefault(TEMP_DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_DOWNLOADS) + "Incomplete" PATH_SEPARATOR_STR);
    setDefault(BIND_ADDRESS, "0.0.0.0");
    setDefault(SLOTS, 5);
    setDefault(TCP_PORT, 30000);
    setDefault(UDP_PORT, 30000);
    setDefault(TLS_PORT, 30001);
    setDefault(INCOMING_CONNECTIONS, INCOMING_DIRECT);
    setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
    setDefault(AUTO_DETECT_CONNECTION, false);
    setDefault(AUTO_FOLLOW, true);
    setDefault(SHARE_HIDDEN, false);
    setDefault(FILTER_MESSAGES, true);
    setDefault(AUTO_SEARCH, true);
    setDefault(AUTO_SEARCH_TIME, 2);
    setDefault(REPORT_ALTERNATES, true);
    setDefault(TIME_STAMPS, true);
    setDefault(IGNORE_HUB_PMS, false);
    setDefault(IGNORE_BOT_PMS, false);
    setDefault(LIST_DUPES, true);
    setDefault(BUFFER_SIZE, 64);
    setDefault(HUBLIST_SERVERS, defaultHubListServers());
    setDefault(DOWNLOAD_SLOTS, 3);
    setDefault(SKIPLIST_SHARE, "*.~*|*.*~");
    setDefault(MAX_DOWNLOAD_SPEED, 0);
    setDefault(LOG_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Logs" PATH_SEPARATOR_STR);
    setDefault(LOG_UPLOADS, false);
    setDefault(LOG_DOWNLOADS, false);
    setDefault(LOG_FINISHED_DOWNLOADS, false);
    setDefault(LOG_PRIVATE_CHAT, false);
    setDefault(LOG_MAIN_CHAT, false);
    setDefault(LOG_SPY, false);
    setDefault(LOG_CMD_DEBUG, false);
    setDefault(UPLOAD_SPEED, connectionSpeeds[11]);
    setDefault(MIN_UPLOAD_SPEED, 0);
    setDefault(LOG_FORMAT_POST_DOWNLOAD, "[%Y-%m-%d %H:%M:%S] %[target] downloaded from %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time], %[fileTR]");
    setDefault(LOG_FORMAT_POST_FINISHED_DOWNLOAD, "%Y-%m-%d %H:%M: %[target] " + string(_("downloaded from")) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIsession]), %[speed], %[time], %[fileTR]");
    setDefault(LOG_FORMAT_POST_UPLOAD,   "[%Y-%m-%d %H:%M:%S] %[source] uploaded to %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time], %[fileTR]");
    setDefault(LOG_FORMAT_MAIN_CHAT,     "[%Y-%m-%d %H:%M:%S] %[message]");
    setDefault(LOG_FORMAT_PRIVATE_CHAT,  "[%Y-%m-%d %H:%M:%S] %[message]");
    setDefault(LOG_FORMAT_STATUS,        "[%Y-%m-%d %H:%M:%S] %[message]");
    setDefault(LOG_FORMAT_SYSTEM,        "[%Y-%m-%d %H:%M:%S] %[message]");
    setDefault(LOG_FORMAT_SPY,        "[%Y-%m-%d %H:%M:%S] %[message] (%[count])");
    setDefault(LOG_FORMAT_CMD_DEBUG,  "[%Y-%m-%d %H:%M:%S] %[type] %[ip]: %[cmd]");
    setDefault(LOG_FILE_MAIN_CHAT,    "CHAT/%B - %Y/%[hubNI] (%[hubURL]).log");
    setDefault(LOG_FILE_STATUS,       "STATUS/%B - %Y/%[hubNI] (%[hubURL]).log");
    setDefault(LOG_FILE_PRIVATE_CHAT, "PM/%B - %Y/%[userNI] (%[userCID]).log");
    setDefault(LOG_FILE_UPLOAD,       "Uploads.log");
    setDefault(LOG_FILE_DOWNLOAD,     "Downloads.log");
    setDefault(LOG_FILE_FINISHED_DOWNLOAD, "Finished_downloads.log");
    setDefault(LOG_FILE_SYSTEM,       "System.log");
    setDefault(LOG_FILE_SPY,          "Spy.log");
    setDefault(LOG_FILE_CMD_DEBUG,    "CmdDebug.log");
    setDefault(SOCKS_PORT, 1080);
    setDefault(SOCKS_RESOLVE, 1);
    setDefault(CONFIG_VERSION, "0.181");        // 0.181 is the last version missing configversion
    setDefault(KEEP_LISTS, false);
    setDefault(AUTO_KICK, false);
    setDefault(COMPRESS_TRANSFERS, true);
    setDefault(SFV_CHECK, true);
    setDefault(DEFAULT_AWAY_MESSAGE, "I'm away. State your business and I might answer later if you're lucky.");
    setDefault(AUTO_AWAY, false);
    setDefault(TIME_STAMPS_FORMAT, "%H:%M");
    setDefault(MAX_COMPRESSION, 6);
    setDefault(NO_AWAYMSG_TO_BOTS, true);
    setDefault(SKIP_ZERO_BYTE, false);
    setDefault(ADLS_BREAK_ON_FIRST, false);
    setDefault(HUB_USER_COMMANDS, true);
    setDefault(AUTO_SEARCH_AUTO_MATCH, false);
    setDefault(LOG_FILELIST_TRANSFERS, false);
    setDefault(LOG_SYSTEM, false);
    setDefault(SEND_UNKNOWN_COMMANDS, true);
    setDefault(MAX_HASH_SPEED, 0);
    setDefault(GET_USER_COUNTRY, true);
    setDefault(LOG_STATUS_MESSAGES, false);
    setDefault(LOG_MAX_FILE_SIZE, 10);
    setDefault(ADD_FINISHED_INSTANTLY, false);
    setDefault(DONT_DL_ALREADY_SHARED, false);
    setDefault(SHOW_LAST_LINES_LOG, 0);
    setDefault(ADC_DEBUG, false);
    setDefault(SEARCH_HISTORY, 10);
    setDefault(SET_MINISLOT_SIZE, 64);
    setDefault(MAX_FILELIST_SIZE, 4*1024);
    setDefault(PRIO_HIGHEST_SIZE, 64);
    setDefault(PRIO_HIGH_SIZE, 0);
    setDefault(PRIO_NORMAL_SIZE, 0);
    setDefault(PRIO_LOW_SIZE, 0);
    setDefault(PRIO_LOWEST, false);
    setDefault(AUTODROP_SPEED, 1024);
    setDefault(AUTODROP_INTERVAL, 10);
    setDefault(AUTODROP_ELAPSED, 15);
    setDefault(AUTODROP_INACTIVITY, 10);
    setDefault(AUTODROP_MINSOURCES, 2);
    setDefault(AUTODROP_FILESIZE, 0);
    setDefault(AUTODROP_ALL, true);
    setDefault(AUTODROP_FILELISTS, true);
    setDefault(AUTODROP_DISCONNECT, true);
    setDefault(NO_IP_OVERRIDE, false);
    setDefault(NO_USE_TEMP_DIR, false);
    setDefault(SHARE_TEMP_FILES, false);
    setDefault(SEARCH_ONLY_FREE_SLOTS, false);
    setDefault(SEARCH_FILTER_SHARED, true);
    setDefault(LAST_SEARCH_TYPE, 0);
    setDefault(SOCKET_IN_BUFFER, 0); // OS default
    setDefault(SOCKET_OUT_BUFFER, 0); // OS default
    setDefault(TLS_TRUSTED_CERTIFICATES_PATH, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR);
    setDefault(TLS_PRIVATE_KEY_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.key");
    setDefault(TLS_CERTIFICATE_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.crt");
    setDefault(USE_TLS, true);
    setDefault(HASHING_START_DELAY, 60); // seconds
    setDefault(AUTO_REFRESH_TIME, 60);
    setDefault(AUTO_SEARCH_LIMIT, 5);
    setDefault(AUTO_KICK_NO_FAVS, false);
    setDefault(PROMPT_PASSWORD, false);
    setDefault(DONT_DL_ALREADY_QUEUED, false);
    setDefault(MAX_COMMAND_LENGTH, 16*1024*1024);
    setDefault(ALLOW_UNTRUSTED_HUBS, true);
    setDefault(ALLOW_UNTRUSTED_CLIENTS, true);
    setDefault(FAST_HASH, true);
    setDefault(SEGMENTED_DL, true);
    setDefault(FOLLOW_LINKS, false);
    setDefault(SEND_BLOOM, true);
    setDefault(FINISHED_DL_ONLY_FULL, true);
    setDefault(SEARCH_MERGE, true);
    setDefault(HASH_BUFFER_SIZE_MB, 8);
    setDefault(HASH_BUFFER_POPULATE, true);
    setDefault(HASH_BUFFER_NORESERVE, true);
    setDefault(HASH_BUFFER_PRIVATE, true);
    setDefault(RECONNECT_DELAY, 15);
    setDefault(DHT_PORT, 6250);
    setDefault(USE_DHT, true);
    setDefault(SEARCH_PASSIVE, false);
    setDefault(MAX_UPLOAD_SPEED_MAIN, 0);
    setDefault(MAX_DOWNLOAD_SPEED_MAIN, 0);
    setDefault(TIME_DEPENDENT_THROTTLE, false);
    setDefault(MAX_DOWNLOAD_SPEED_ALTERNATE, 0);
    setDefault(MAX_UPLOAD_SPEED_ALTERNATE, 0);
    setDefault(BANDWIDTH_LIMIT_START, 1);
    setDefault(BANDWIDTH_LIMIT_END, 1);
    setDefault(SLOTS_ALTERNATE_LIMITING, 1);
    setDefault(SLOTS_PRIMARY, 3);
    setDefault(THROTTLE_ENABLE, false);
    setDefault(KEEP_FINISHED_FILES, false);
    setDefault(USE_IP, true);
    setDefault(SHOW_FREE_SLOTS_DESC, false);
    setDefault(OVERLAP_CHUNKS, true);
    setDefault(CASESENSITIVE_FILELIST, false);
    setDefault(IPFILTER,false);
    setDefault(USE_LUA,false);
    setDefault(ALLOW_NATT, true);
    setDefault(IP_TOS_VALUE, -1);
    setDefault(SEGMENT_SIZE, 0);
    setDefault(BIND_IFACE, false);
    setDefault(BIND_IFACE_NAME, "");
    setDefault(MINIMUM_SEARCH_INTERVAL, 60);
    setDefault(DYNDNS_SERVER, "http://checkip.dyndns.org/index.html");
    setDefault(DYNDNS_ENABLE, false);
    setDefault(ALLOW_UPLOAD_MULTI_HUB, true);
    setDefault(USE_ADL_ONLY_OWN_LIST, false);
    setDefault(ALLOW_SIM_UPLOADS, true);
    setDefault(CHECK_TARGETS_PATHS_ON_START, false);
    setDefault(SHARE_SKIP_ZERO_BYTE, false);
    setDefault(APP_UNIT_BASE, 0);
    setDefault(REQUIRE_TLS, true); // True by default: We assume TLS is commonplace enough among ADC clients.

    setSearchTypeDefaults();
}

} // namespace dcpp
