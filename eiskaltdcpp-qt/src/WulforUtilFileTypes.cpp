/***************************************************************************
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
    m_FileTypeMap["A52"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AAC"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AC3"]  = eiFILETYPE_MP3;
    m_FileTypeMap["APE"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AIFF"] = eiFILETYPE_MP3;
    m_FileTypeMap["AU"]   = eiFILETYPE_MP3;
    m_FileTypeMap["DTS"]  = eiFILETYPE_MP3;
    m_FileTypeMap["FLA"]  = eiFILETYPE_MP3;
    m_FileTypeMap["FLAC"] = eiFILETYPE_MP3;
    m_FileTypeMap["MID"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MOD"]  = eiFILETYPE_MP3;
    m_FileTypeMap["M4A"]  = eiFILETYPE_MP3;
    m_FileTypeMap["M4P"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MPC"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MP1"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MP2"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MP3"]  = eiFILETYPE_MP3;
    m_FileTypeMap["OGG"]  = eiFILETYPE_MP3;
    m_FileTypeMap["RA"]   = eiFILETYPE_MP3;
    m_FileTypeMap["SHN"]  = eiFILETYPE_MP3;
    m_FileTypeMap["SPX"]  = eiFILETYPE_MP3;
    m_FileTypeMap["WAV"]  = eiFILETYPE_MP3;
    m_FileTypeMap["WMA"]  = eiFILETYPE_MP3;
    m_FileTypeMap["WV"]   = eiFILETYPE_MP3;
    m_FileTypeMap["669"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AIF"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AMF"]  = eiFILETYPE_MP3;
    m_FileTypeMap["AMS"]  = eiFILETYPE_MP3;
    m_FileTypeMap["DBM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["DMF"]  = eiFILETYPE_MP3;
    m_FileTypeMap["DSM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["FAR"]  = eiFILETYPE_MP3;
    m_FileTypeMap["IT"]   = eiFILETYPE_MP3;
    m_FileTypeMap["MDL"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MED"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MIDI"] = eiFILETYPE_MP3;
    m_FileTypeMap["MOL"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MPA"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MPP"]  = eiFILETYPE_MP3;
    m_FileTypeMap["MTM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["NST"]  = eiFILETYPE_MP3;
    m_FileTypeMap["OKT"]  = eiFILETYPE_MP3;
    m_FileTypeMap["PSM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["PTM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["RMI"]  = eiFILETYPE_MP3;
    m_FileTypeMap["S3M"]  = eiFILETYPE_MP3;
    m_FileTypeMap["STM"]  = eiFILETYPE_MP3;
    m_FileTypeMap["ULT"]  = eiFILETYPE_MP3;
    m_FileTypeMap["UMX"]  = eiFILETYPE_MP3;
    m_FileTypeMap["WOW"]  = eiFILETYPE_MP3;
    m_FileTypeMap["XM"]   = eiFILETYPE_MP3;


    // ARCHIVE 3
    m_FileTypeMap["7Z"]  = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ACE"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ARJ"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["BZ2"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CAB"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["EX_"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["GZ"]  = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["HQX"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["JAR"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ISO"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["MDF"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["MDS"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["NRG"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["LZH"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["LHA"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["RAR"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["RPM"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["SEA"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["TAR"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["TGZ"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["VCD"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["BWT"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CCD"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CDI"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["PDI"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["CUE"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ISZ"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["IMG"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["VC4"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["UC2"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ZIP"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["ZOO"] = eiFILETYPE_ARCHIVE;
    m_FileTypeMap["Z"]   = eiFILETYPE_ARCHIVE;

    // DOCUMENT 4
    m_FileTypeMap["CFG"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CHM"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CONF"]  = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CPP"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["CSS"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["C"]     = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DIZ"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DOC"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["DOCX"]  = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["H"]     = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HLP"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HTM"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["HTML"]  = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["INI"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["INF"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["LOG"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["NFO"]   = eiFILETYPE_DOCUMENT;
    m_FileTypeMap["ODG"]   = eiFILETYPE_DOCUMENT;
    wulforRegisterExtraFileTypes(m_FileTypeMap);
}
