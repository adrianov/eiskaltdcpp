/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"

#include <QMap>
#include <QString>

using Icons = WulforUtil::Icons;
static const Icons eiFILETYPE_DOCUMENT = WulforUtil::eiFILETYPE_DOCUMENT;
static const Icons eiFILETYPE_APPLICATION = WulforUtil::eiFILETYPE_APPLICATION;
static const Icons eiFILETYPE_ARCHIVE = WulforUtil::eiFILETYPE_ARCHIVE;
static const Icons eiFILETYPE_PICTURE = WulforUtil::eiFILETYPE_PICTURE;
static const Icons eiFILETYPE_VIDEO = WulforUtil::eiFILETYPE_VIDEO;
static const Icons eiFILETYPE_MP3 = WulforUtil::eiFILETYPE_MP3;
static const Icons eiFILETYPE_UNKNOWN = WulforUtil::eiFILETYPE_UNKNOWN;

void wulforRegisterExtraFileTypes(QMap<QString, WulforUtil::Icons> &map)
{
    map["ODP"]   = eiFILETYPE_DOCUMENT;
    map["ODS"]   = eiFILETYPE_DOCUMENT;
    map["ODT"]   = eiFILETYPE_DOCUMENT;
    map["PDF"]   = eiFILETYPE_DOCUMENT;
    map["PHP"]   = eiFILETYPE_DOCUMENT;
    map["PPT"]   = eiFILETYPE_DOCUMENT;
    map["PS"]    = eiFILETYPE_DOCUMENT;
    map["PDF"]   = eiFILETYPE_DOCUMENT;
    map["SHTML"] = eiFILETYPE_DOCUMENT;
    map["SXC"]   = eiFILETYPE_DOCUMENT;
    map["SXD"]   = eiFILETYPE_DOCUMENT;
    map["SXI"]   = eiFILETYPE_DOCUMENT;
    map["SXW"]   = eiFILETYPE_DOCUMENT;
    map["TXT"]   = eiFILETYPE_DOCUMENT;
    map["RFT"]   = eiFILETYPE_DOCUMENT;
    map["RDF"]   = eiFILETYPE_DOCUMENT;
    map["RTF"]   = eiFILETYPE_DOCUMENT;
    map["XML"]   = eiFILETYPE_DOCUMENT;
    map["XLS"]   = eiFILETYPE_DOCUMENT;

    // APPL 5
    map["BAT"] = eiFILETYPE_APPLICATION;
    map["CGI"] = eiFILETYPE_APPLICATION;
    map["COM"] = eiFILETYPE_APPLICATION;
    map["DLL"] = eiFILETYPE_APPLICATION;
    map["EXE"] = eiFILETYPE_APPLICATION;
    map["HQX"] = eiFILETYPE_APPLICATION;
    map["JS"]  = eiFILETYPE_APPLICATION;
    map["SH"]  = eiFILETYPE_APPLICATION;
    map["SO"]  = eiFILETYPE_APPLICATION;
    map["SYS"] = eiFILETYPE_APPLICATION;
    map["VXD"] = eiFILETYPE_APPLICATION;
    map["MSI"] = eiFILETYPE_APPLICATION;

    // PICTURE 6
    map["3DS"]  = eiFILETYPE_PICTURE;
    map["A11"]  = eiFILETYPE_PICTURE;
    map["ACB"]  = eiFILETYPE_PICTURE;
    map["ADC"]  = eiFILETYPE_PICTURE;
    map["ADI"]  = eiFILETYPE_PICTURE;
    map["AFI"]  = eiFILETYPE_PICTURE;
    map["AI"]   = eiFILETYPE_PICTURE;
    map["AIS"]  = eiFILETYPE_PICTURE;
    map["ANS"]  = eiFILETYPE_PICTURE;
    map["ART"]  = eiFILETYPE_PICTURE;
    map["B8"]   = eiFILETYPE_PICTURE;
    map["BMP"]  = eiFILETYPE_PICTURE;
    map["CBM"]  = eiFILETYPE_PICTURE;
    map["DCX"]  = eiFILETYPE_PICTURE;
    map["EPS"]  = eiFILETYPE_PICTURE;
    map["EMF"]  = eiFILETYPE_PICTURE;
    map["GIF"]  = eiFILETYPE_PICTURE;
    map["ICO"]  = eiFILETYPE_PICTURE;
    map["IMG"]  = eiFILETYPE_PICTURE;
    map["JPEG"] = eiFILETYPE_PICTURE;
    map["JPE"]  = eiFILETYPE_PICTURE;
    map["JPG"]  = eiFILETYPE_PICTURE;
    map["PCT"]  = eiFILETYPE_PICTURE;
    map["PCX"]  = eiFILETYPE_PICTURE;
    map["PIC"]  = eiFILETYPE_PICTURE;
    map["PICT"] = eiFILETYPE_PICTURE;
    map["PNG"]  = eiFILETYPE_PICTURE;
    map["PS"]   = eiFILETYPE_PICTURE;
    map["PSD"]  = eiFILETYPE_PICTURE;
    map["PSP"]  = eiFILETYPE_PICTURE;
    map["RLE"]  = eiFILETYPE_PICTURE;
    map["TGA"]  = eiFILETYPE_PICTURE;
    map["TIF"]  = eiFILETYPE_PICTURE;
    map["TIFF"] = eiFILETYPE_PICTURE;
    map["XPM"]  = eiFILETYPE_PICTURE;
    map["XIF"]  = eiFILETYPE_PICTURE;
    map["WMF"]  = eiFILETYPE_PICTURE;

    // VIDEO 7
    map["AVI"]   = eiFILETYPE_VIDEO;
    map["ASF"]   = eiFILETYPE_VIDEO;
    map["ASX"]   = eiFILETYPE_VIDEO;
    map["DAT"]   = eiFILETYPE_VIDEO;
    map["DIVX"]  = eiFILETYPE_VIDEO;
    map["DV"]    = eiFILETYPE_VIDEO;
    map["FLV"]   = eiFILETYPE_VIDEO;
    map["M1V"]   = eiFILETYPE_VIDEO;
    map["M2V"]   = eiFILETYPE_VIDEO;
    map["M4V"]   = eiFILETYPE_VIDEO;
    map["MKV"]   = eiFILETYPE_VIDEO;
    map["MOV"]   = eiFILETYPE_VIDEO;
    map["MOVIE"] = eiFILETYPE_VIDEO;
    map["MP4"]   = eiFILETYPE_VIDEO;
    map["MPE"]   = eiFILETYPE_VIDEO;
    map["MPEG"]  = eiFILETYPE_VIDEO;
    map["MPEG1"] = eiFILETYPE_VIDEO;
    map["MPEG2"] = eiFILETYPE_VIDEO;
    map["MPEG4"] = eiFILETYPE_VIDEO;
    map["MP1V"]  = eiFILETYPE_VIDEO;
    map["MP2V"]  = eiFILETYPE_VIDEO;
    map["MPV1"]  = eiFILETYPE_VIDEO;
    map["MPV2"]  = eiFILETYPE_VIDEO;
    map["MPG"]   = eiFILETYPE_VIDEO;
    map["MPS"]   = eiFILETYPE_VIDEO;
    map["MPV"]   = eiFILETYPE_VIDEO;
    map["OGM"]   = eiFILETYPE_VIDEO;
    map["PXP"]   = eiFILETYPE_VIDEO;
    map["QT"]    = eiFILETYPE_VIDEO;
    map["RAM"]   = eiFILETYPE_VIDEO;
    map["RM"]    = eiFILETYPE_VIDEO;
    map["RV"]    = eiFILETYPE_VIDEO;
    map["RMVB"]  = eiFILETYPE_VIDEO;
    map["VIV"]   = eiFILETYPE_VIDEO;
    map["VIVO"]  = eiFILETYPE_VIDEO;
    map["VOB"]   = eiFILETYPE_VIDEO;
    map["WMV"]   = eiFILETYPE_VIDEO;
    map["TS"]    = eiFILETYPE_VIDEO;
}
