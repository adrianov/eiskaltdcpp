#!/bin/sh
set -e

root=$(cd "$(dirname "$0")/.." && pwd)
build=$root/build
dist=$root/dist

export HOMEBREW=${HOMEBREW:-$(brew --prefix)}
export OSX_ARCHITECTURES=${OSX_ARCHITECTURES:-arm64}
export OSX_DEPLOYMENT_TARGET=${OSX_DEPLOYMENT_TARGET:-26.0}

# Apple Clang rejects -fuse-ld=mold (often set for Linux). Use stock clang.
unset CC CXX CFLAGS CXXFLAGS LDFLAGS
export CC=clang
export CXX=clang++

# Drop a poisoned in-tree cache from a previous configure in the source root.
rm -rf "$root/CMakeCache.txt" "$root/CMakeFiles"

mkdir -p "$build"
cd "$build"

cmake "$root" \
    -DCMAKE_TOOLCHAIN_FILE="$root/macos/homebrew-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="$dist" \
    -DCMAKE_INSTALL_MESSAGE=NEVER \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++

make -j"$(sysctl -n hw.ncpu)"
make install

app=$dist/EiskaltDC++.app
aspell=$app/Contents/Resources/aspell

if [ ! -d "$aspell/dict" ]; then
    tmp=$(mktemp -d)
    curl -fsSL -o "$tmp/aspell.zip" \
        "https://sourceforge.net/projects/eiskaltdcpp/files/Other/aspell.zip/download"
    unzip -q "$tmp/aspell.zip" -d "$tmp"
    cp -R "$tmp/aspell" "$app/Contents/Resources/"
    rm -rf "$tmp"
fi

# Any post-sign edit of Mach-O / sealed resources causes CODESIGNING Invalid Page at launch.
xattr -cr "$app" 2>/dev/null || true
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
open "$app"
