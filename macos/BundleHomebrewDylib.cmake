# Bundle a single dylib into a macOS .app (Contents/Frameworks) and rewrite load paths.
# Configure-time / -D vars:
#   BUNDLE_APP_NAME   - e.g. EiskaltDC++
#   BUNDLE_DYLIB_SRC  - path passed to the linker (may be a symlink)
#   BUNDLE_DYLIB_REAL - REALPATH of the dylib
#   BUNDLE_DYLIB_NAME - file name inside Frameworks (e.g. libduckdb.dylib)
#   BUNDLE_DYLIB_EXTRA_OLDS - optional ;-list of alternate absolute paths
#   CMAKE_INSTALL_PREFIX - parent of ${BUNDLE_APP_NAME}.app

if (NOT BUNDLE_APP_NAME OR NOT BUNDLE_DYLIB_REAL OR NOT BUNDLE_DYLIB_NAME)
  message(FATAL_ERROR "BundleHomebrewDylib: BUNDLE_APP_NAME / BUNDLE_DYLIB_REAL / BUNDLE_DYLIB_NAME required")
endif ()

get_filename_component(_bundle_prefix "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
set(_bundle_app "${_bundle_prefix}/${BUNDLE_APP_NAME}.app")
get_filename_component(_bundle_app "${_bundle_app}" ABSOLUTE)
set(_bundle_fw "${_bundle_app}/Contents/Frameworks")
set(_bundle_bin "${_bundle_app}/Contents/MacOS/${BUNDLE_APP_NAME}")
set(_bundle_dest "${_bundle_fw}/${BUNDLE_DYLIB_NAME}")
set(_bundle_id "@executable_path/../Frameworks/${BUNDLE_DYLIB_NAME}")

if (NOT EXISTS "${_bundle_bin}")
  message(FATAL_ERROR "BundleHomebrewDylib: binary not found: ${_bundle_bin}")
endif ()
if (NOT EXISTS "${BUNDLE_DYLIB_REAL}")
  message(FATAL_ERROR "BundleHomebrewDylib: dylib not found: ${BUNDLE_DYLIB_REAL}")
endif ()

file(MAKE_DIRECTORY "${_bundle_fw}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${BUNDLE_DYLIB_REAL}" "${_bundle_dest}"
  RESULT_VARIABLE _bundle_rc)
if (_bundle_rc)
  message(FATAL_ERROR "BundleHomebrewDylib: copy failed (${_bundle_rc})")
endif ()

execute_process(COMMAND install_name_tool -id "${_bundle_id}" "${_bundle_dest}"
  RESULT_VARIABLE _bundle_rc
  ERROR_VARIABLE _bundle_err)
if (_bundle_rc)
  message(FATAL_ERROR "BundleHomebrewDylib: -id failed: ${_bundle_err}")
endif ()

# Ad-hoc signature blocks install_name_tool on Apple Silicon.
execute_process(COMMAND codesign --remove-signature "${_bundle_bin}" ERROR_QUIET)
execute_process(COMMAND codesign --remove-signature "${_bundle_dest}" ERROR_QUIET)

# Discover the exact LC_LOAD_DYLIB path currently recorded for this dylib.
execute_process(COMMAND otool -L "${_bundle_bin}"
  OUTPUT_VARIABLE _bundle_otool
  ERROR_QUIET)
set(_bundle_olds "${BUNDLE_DYLIB_SRC};${BUNDLE_DYLIB_REAL}")
if (BUNDLE_DYLIB_EXTRA_OLDS)
  list(APPEND _bundle_olds ${BUNDLE_DYLIB_EXTRA_OLDS})
endif ()
string(REGEX MATCHALL "[^\n\t ]+/${BUNDLE_DYLIB_NAME}" _otool_refs "${_bundle_otool}")
list(APPEND _bundle_olds ${_otool_refs})
list(REMOVE_DUPLICATES _bundle_olds)

set(_changed FALSE)
foreach (_old IN LISTS _bundle_olds)
  if (_old AND NOT _old MATCHES "^@")
    execute_process(COMMAND install_name_tool -change "${_old}" "${_bundle_id}" "${_bundle_bin}"
      RESULT_VARIABLE _chg_rc
      ERROR_VARIABLE _chg_err)
    if (NOT _chg_rc)
      set(_changed TRUE)
    endif ()
  endif ()
endforeach ()

execute_process(COMMAND install_name_tool
  -add_rpath "@executable_path/../Frameworks" "${_bundle_bin}"
  ERROR_QUIET)

execute_process(COMMAND codesign -s - --force --deep "${_bundle_app}" ERROR_QUIET)

execute_process(COMMAND otool -L "${_bundle_bin}"
  OUTPUT_VARIABLE _bundle_otool2
  ERROR_QUIET)
string(REGEX MATCH "[^\n]*${BUNDLE_DYLIB_NAME}[^\n]*" _duck_line "${_bundle_otool2}")
if (NOT _duck_line MATCHES "@executable_path/../Frameworks/${BUNDLE_DYLIB_NAME}")
  message(FATAL_ERROR
    "BundleHomebrewDylib: ${BUNDLE_DYLIB_NAME} not rewritten to @executable_path "
    "(changed=${_changed}). line=[${_duck_line}] tried=[${_bundle_olds}]")
endif ()

message(STATUS "Bundled ${BUNDLE_DYLIB_NAME} into ${_bundle_app}")
