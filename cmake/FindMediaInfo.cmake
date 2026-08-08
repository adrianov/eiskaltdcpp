# Find MediaInfoLib (libmediainfo + MediaInfo/MediaInfo.h).
# Sets MediaInfo_FOUND, MediaInfo_INCLUDE_DIRS, MediaInfo_LIBRARIES.

if (MediaInfo_INCLUDE_DIR AND MediaInfo_LIBRARY)
  set(MediaInfo_FIND_QUIETLY TRUE)
endif ()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MEDIAINFO QUIET libmediainfo)
endif ()

find_path(MediaInfo_INCLUDE_DIR NAMES MediaInfo/MediaInfo.h
  HINTS ${PC_MEDIAINFO_INCLUDE_DIRS}
  PATH_SUFFIXES include)

find_library(MediaInfo_LIBRARY NAMES mediainfo libmediainfo
  HINTS ${PC_MEDIAINFO_LIBRARY_DIRS})

find_library(Zen_LIBRARY NAMES zen libzen
  HINTS ${PC_MEDIAINFO_LIBRARY_DIRS})

set(MediaInfo_LIBRARIES ${MediaInfo_LIBRARY})
if (Zen_LIBRARY)
  list(APPEND MediaInfo_LIBRARIES ${Zen_LIBRARY})
endif ()
if (MediaInfo_INCLUDE_DIR)
  set(MediaInfo_INCLUDE_DIRS ${MediaInfo_INCLUDE_DIR})
endif ()
if (PC_MEDIAINFO_INCLUDE_DIRS)
  list(APPEND MediaInfo_INCLUDE_DIRS ${PC_MEDIAINFO_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES MediaInfo_INCLUDE_DIRS)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MediaInfo DEFAULT_MSG MediaInfo_LIBRARY MediaInfo_INCLUDE_DIR)

mark_as_advanced(MediaInfo_INCLUDE_DIR MediaInfo_LIBRARY Zen_LIBRARY)
