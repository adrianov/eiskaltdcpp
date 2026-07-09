/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

namespace dcpp {

StringList SettingsManager::connectionSpeeds;

const string SettingsManager::settingTags[] =
{
    // Strings
    "Nick", "UploadSpeed", "Description", "DownloadDirectory", "EMail",
    "ExternalIp", "HublistServers", "HttpProxy",
    "LogDirectory", "LogFormatPostDownload","LogFormatPostFinishedDownload",
    "LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat",
    "TempDownloadDirectory", "BindAddress", "SocksServer",
    "SocksUser", "SocksPassword", "ConfigVersion", "DefaultAwayMessage",
    "TimeStampsFormat", "CID", "LogFileMainChat", "LogFilePrivateChat",
    "LogFileStatus", "LogFileUpload", "LogFileDownload", "LogFileFinishedDownload",
    "LogFileSystem",
    "LogFormatSystem", "LogFormatStatus", "LogFileSpy", "LogFormatSpy", "TLSPrivateKeyFile",
    "TLSCertificateFile", "TLSTrustedCertificatesPath",
    "Language", "SkipListShare", "InternetIp", "BindIfaceName",
    "DHTKey", "DynDNSServer", "MimeHandler",
    "LogFileCmdDebug", "LogFormatCmdDebug",
    "SENTRY",
    // Ints
    "IncomingConnections", "InPort", "Slots", "AutoFollow",
    "ShareHidden", "FilterMessages", "AutoSearch",
    "AutoSearchTime", "ReportFoundAlternates", "TimeStamps",
    "IgnoreHubPms", "IgnoreBotPms",
    "ListDuplicates", "BufferSize", "DownloadSlots", "MaxDownloadSpeed",
    "LogMainChat", "LogPrivateChat", "LogDownloads","LogFileFinishedDownload",
    "LogUploads", "MinUploadSpeed", "AutoAway",
    "SocksPort", "SocksResolve", "KeepLists", "AutoKick",
    "CompressTransfers", "SFVCheck",
    "MaxCompression", "NoAwayMsgToBots", "SkipZeroByte", "AdlsBreakOnFirst",
    "HubUserCommands", "AutoSearchAutoMatch","LogSystem",
    "LogFilelistTransfers",
    "SendUnknownCommands", "MaxHashSpeed",
    "GetUserCountry", "LogStatusMessages", "SearchPassiveAlways",
    "AddFinishedInstantly", "DontDLAlreadyShared",
    "UDPPort", "ShowLastLinesLog", "AdcDebug",
    "SearchHistory", "SetMinislotSize",
    "MaxFilelistSize", "HighestPrioSize", "HighPrioSize", "NormalPrioSize",
    "LowPrioSize", "LowestPrio", "AutoDropSpeed", "AutoDropInterval",
    "AutoDropElapsed", "AutoDropInactivity", "AutoDropMinSources",
    "AutoDropFilesize", "AutoDropAll", "AutoDropFilelists",
    "AutoDropDisconnect", "OutgoingConnections", "NoIpOverride", "NoUseTempDir",
    "ShareTempFiles", "SearchOnlyFreeSlots", "LastSearchType",
    "SocketInBuffer", "SocketOutBuffer",
    "AutoRefreshTime", "HashingStartDelay", "UseTLS", "AutoSearchLimit",
    "AutoKickNoFavs", "PromptPassword",
    "DontDlAlreadyQueued", "MaxCommandLength", "AllowUntrustedHubs",
    "AllowUntrustedClients", "TLSPort", "FastHash",
    "SegmentedDL", "FollowLinks", "SendBloom",
    "SearchFilterShared", "FinishedDLOnlyFull",
    "SearchMerge", "HashBufferSize", "HashBufferPopulate",
    "HashBufferNoReserve", "HashBufferPrivate",
    "UseDHT", "DHTPort",
    "ReconnectDelay", "AutoDetectIncomingConnection",
    "BandwidthLimitStart", "BandwidthLimitEnd", "EnableThrottle","TimeDependentThrottle",
    "MaxDownloadSpeedAlternate", "MaxUploadSpeedAlternate",
    "MaxDownloadSpeedMain", "MaxUploadSpeedMain",
    "SlotsAlternateLimiting", "SlotsPrimaryLimiting", "KeepFinishedFiles",
    "ShowFreeSlotsDesc", "UseIP", "OverLapChunks", "CaseSensitiveFilelist",
    "IpFilter", "TextColor", "UseLua", "AllowNatt", "IpTOSValue", "SegmentSize",
    "BindIface", "MinimumSearchInterval", "EnableDynDNS", "AllowUploadOverMultiHubs",
    "UseADLOnlyOnOwnList", "AllowSimUploads", "CheckTargetsPathsOnStart", "NmdcDebug",
    "ShareSkipZeroByte", "RequireTLS", "LogSpy", "AppUnitBase",
    "LogCmdDebug", "LogMaxFileSize",
    "SENTRY",
    // Int64
    "TotalUpload", "TotalDownload",
    "SENTRY",
    // Floats
    "SENTRY"
};

SettingsManager::SettingsManager()
{

    connectionSpeeds.push_back("0.005");
    connectionSpeeds.push_back("0.01");
    connectionSpeeds.push_back("0.02");
    connectionSpeeds.push_back("0.05");
    connectionSpeeds.push_back("0.1");
    connectionSpeeds.push_back("0.2");
    connectionSpeeds.push_back("0.5");
    connectionSpeeds.push_back("1");
    connectionSpeeds.push_back("2");
    connectionSpeeds.push_back("5");
    connectionSpeeds.push_back("10");
    connectionSpeeds.push_back("20");
    connectionSpeeds.push_back("50");
    connectionSpeeds.push_back("100");
    connectionSpeeds.push_back("1000");

    for(int i=0; i<SETTINGS_LAST; i++)
        isSet[i] = false;

    for(int i=0; i<INT_LAST-INT_FIRST; i++) {
        intDefaults[i] = 0;
        intSettings[i] = 0;
    }
    for(int i=0; i<INT64_LAST-INT64_FIRST; i++) {
        int64Defaults[i] = 0;
        int64Settings[i] = 0;
    }
    for(int i=0; i<FLOAT_LAST-FLOAT_FIRST; i++) {
        floatDefaults[i] = 0;
        floatSettings[i] = 0;
    }

    setDefaults();
}

} // namespace dcpp
