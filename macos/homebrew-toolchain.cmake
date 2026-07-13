if(DEFINED ENV{HOMEBREW})
    set(HOMEBREW "$ENV{HOMEBREW}")
else()
    set(HOMEBREW "/usr/local")
endif()

if(DEFINED ENV{OSX_DEPLOYMENT_TARGET})
    set(OSX_DEPLOYMENT_TARGET "$ENV{OSX_DEPLOYMENT_TARGET}")
else()
    set(OSX_DEPLOYMENT_TARGET "26.0")
endif()

if(DEFINED ENV{OSX_ARCHITECTURES})
    set(OSX_ARCHITECTURES "$ENV{OSX_ARCHITECTURES}")
else()
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(OSX_ARCHITECTURES "arm64")
    else()
        set(OSX_ARCHITECTURES "x86_64")
    endif()
endif()

# Absolute Apple Clang — bare "clang" resolves to ccache/libexec when that is on PATH,
# which flips CMAKE_*_COMPILER mid-configure, wipes the cache, and drops Qt prefixes.
set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE FILEPATH "" FORCE)

set(_HB_PREFIX
    "${HOMEBREW}"
    "${HOMEBREW}/opt/gettext"
    "${HOMEBREW}/opt/openssl@1.1"
    "${HOMEBREW}/opt/openssl"
    "${HOMEBREW}/opt/qt@5"
    "${HOMEBREW}/opt/qt5"
    "${HOMEBREW}/opt/qt"
    "${HOMEBREW}/opt/duckdb")
set(CMAKE_PREFIX_PATH "${_HB_PREFIX}" CACHE STRING "Homebrew package prefixes" FORCE)
unset(_HB_PREFIX)

set(CMAKE_OSX_ARCHITECTURES "${OSX_ARCHITECTURES}"
    CACHE STRING "CMAKE_OSX_ARCHITECTURES" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "${OSX_DEPLOYMENT_TARGET}"
    CACHE STRING "CMAKE_OSX_DEPLOYMENT_TARGET" FORCE)
