/***************************************************************************
*                                                                         *
*   Copyright (C) 2026 Peter Adrianov <peter.adrianov@gmail.com>          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "WulforUtil.h"

void wulforRegisterExtraFileTypes(QMap<QString, WulforUtil::Icons> &map);

void WulforUtil::initFileTypes(){
    m_FileTypeMap.clear();

    // MP3 2
    m_FileTypeMap["A52"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AAC"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AC3"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["APE"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AIFF"] = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AU"]   = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["DTS"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["FLA"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["FLAC"] = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MID"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MOD"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["M4A"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["M4P"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MPC"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MP1"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MP2"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MP3"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["OGG"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["RA"]   = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["SHN"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["SPX"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["WAV"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["WMA"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["WV"]   = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["669"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AIF"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AMF"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["AMS"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["DBM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["DMF"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["DSM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["FAR"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["IT"]   = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MDL"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MED"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MIDI"] = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MOL"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MPA"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MPP"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["MTM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["NST"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["OKT"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["PSM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["PTM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["RMI"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["S3M"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["STM"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["ULT"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["UMX"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["WOW"]  = AppIcons::eiFILETYPE_MP3;
    m_FileTypeMap["XM"]   = AppIcons::eiFILETYPE_MP3;


    // ARCHIVE 3
    m_FileTypeMap["7Z"]  = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ACE"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ARJ"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["BZ2"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CAB"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["EX_"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["GZ"]  = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["HQX"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["JAR"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ISO"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["MDF"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["MDS"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["NRG"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["LZH"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["LHA"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["RAR"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["RPM"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["SEA"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["TAR"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["TGZ"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["VCD"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["BWT"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CCD"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CDI"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["PDI"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CUE"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ISZ"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["IMG"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["VC4"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["UC2"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ZIP"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ZOO"] = AppIcons::eiFILETYPE_ARCHIVE;
    m_FileTypeMap["Z"]   = AppIcons::eiFILETYPE_ARCHIVE;

    // DOCUMENT 4
    m_FileTypeMap["CFG"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CHM"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CONF"]  = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CPP"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CSS"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["C"]     = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DIZ"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DOC"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DOCX"]  = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["H"]     = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HLP"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HTM"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HTML"]  = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["INI"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["INF"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["LOG"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["NFO"]   = AppIcons::eiFILETYPE_DOCUMENT;
    m_FileTypeMap["ODG"]   = AppIcons::eiFILETYPE_DOCUMENT;
    wulforRegisterExtraFileTypes(m_FileTypeMap);
}
