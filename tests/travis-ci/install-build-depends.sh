#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

if [ "${TARGET}" = "linux64" ]
then
    sudo apt-get update  -qq
    sudo apt-get install -qq cmake \
                             libbz2-dev \
                             libssl-dev \
                             libattr1-dev \
                             zlib1g-dev \
                             libidn2-dev \
                             liblua5.1-0-dev \
                             libpcre2-dev \
                             libminiupnpc-dev

    if [ "${USE_QT}" = "qt6" ]
    then
        sudo apt-get install -qq qt6-base-dev \
                                 qt6-tools-dev \
                                 qt6-multimedia-dev \
                                 qt6-tools-dev-tools \
                                 libqt6core5compat6-dev \
                                 libaspell-dev
    fi

    if [ "${USE_GTK}" = "gtk2" ]
    then
        sudo apt-get install -qq libgtk2.0-dev \
                                 libnotify-dev \
                                 libcanberra-gtk-dev
    elif [ "${USE_GTK}" = "gtk3" ]
    then
        sudo apt-get install -qq libgtk-3-dev \
                                 libnotify-dev \
                                 libcanberra-gtk3-dev
    fi

    if [ "${USE_DAEMON}" = "jsonrpc" ]
    then
        sudo apt-get install -qq libjsoncpp-dev \
                                 libdata-dump-perl \
                                 libgetopt-long-descriptive-perl \
                                 libjson-rpc-perl \
                                 libterm-shellui-perl
    fi
fi

if [ "${TARGET}" = "windows32" ] || [ "${TARGET}" = "windows64" ]
then
    # Add Debian packages built from MXE packages
    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys C6BF758A33A3A276
    sudo add-apt-repository 'deb [arch=amd64] https://mirror.mxe.cc/repos/apt focal main'

    if [ "${TARGET}" = "windows64" ]
    then
        PKG_PREFIX="mxe-x86-64-w64-mingw32.shared"
    else
        PKG_PREFIX="mxe-i686-w64-mingw32.shared"
    fi

    sudo apt-get update  -qq
    sudo apt-get install -qq ${PKG_PREFIX}-aspell \
                             ${PKG_PREFIX}-jsoncpp \
                             ${PKG_PREFIX}-libidn2 \
                             ${PKG_PREFIX}-lua \
                             ${PKG_PREFIX}-miniupnpc \
                             ${PKG_PREFIX}-qtbase \
                             ${PKG_PREFIX}-qttools \
                             ${PKG_PREFIX}-qttranslations \
                             ${PKG_PREFIX}-qtmultimedia
fi

if [ "${TARGET}" = "macos64" ]
then
    # export HOMEBREW_NO_AUTO_UPDATE=1
    export HOMEBREW_NO_BOTTLE_SOURCE_FALLBACK=1
    # brew install cmake gettext qt qt5compat
    brew install ccache coreutils aspell jsoncpp libidn2 lua miniupnpc
fi
