# Find DuckDB (libduckdb + duckdb.hpp). Sets DuckDB_FOUND, DuckDB_INCLUDE_DIRS, DuckDB_LIBRARIES.

if (DuckDB_INCLUDE_DIR AND DuckDB_LIBRARY)
  set(DuckDB_FIND_QUIETLY TRUE)
endif ()

find_path(DuckDB_INCLUDE_DIR NAMES duckdb.hpp duckdb.h
  PATH_SUFFIXES include)
find_library(DuckDB_LIBRARY NAMES duckdb libduckdb)

if (DuckDB_LIBRARY)
  set(DuckDB_LIBRARIES ${DuckDB_LIBRARY})
endif ()
if (DuckDB_INCLUDE_DIR)
  set(DuckDB_INCLUDE_DIRS ${DuckDB_INCLUDE_DIR})
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DuckDB DEFAULT_MSG DuckDB_LIBRARIES DuckDB_INCLUDE_DIR)

mark_as_advanced(DuckDB_INCLUDE_DIR DuckDB_LIBRARY)
