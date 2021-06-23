#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jsoncpp"
readonly ownership="JsonCpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/jsoncpp.git"
readonly tag="for/svtk-20200302-1.9.2"
readonly paths="
.gitattributes
CMakeLists.svtk.txt
README.kitware.md
LICENSE
" # We amalgamate jsoncpp

extract_source () {
    python2 amalgamate.py
    [ -n "$paths" ] && \
        mv -v $paths "dist"
    mv "json/svtkjsoncpp_config.h.in" "dist/json"
    mv "dist/CMakeLists.svtk.txt" "dist/CMakeLists.txt"
    mv "dist" "$name-reduced"
    tar -cv "$name-reduced/" | \
        tar -C "$extractdir" -x
}

. "${BASH_SOURCE%/*}/../update-common.sh"
