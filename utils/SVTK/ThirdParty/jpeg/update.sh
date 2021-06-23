#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jpeg"
readonly ownership="jpeg-turbo Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libjpeg-turbo.git"
readonly tag="for/svtk-20191230-2.0.3"
readonly paths="
.gitattributes
CMakeLists.svtk.txt

j*.c
j*.h
svtk_jpeg_mangle.h
jconfig.h.in
jconfigint.h.in
win/jconfig.h.in

LICENSE.md
README.ijg
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -v *-tj.c
    mv -v CMakeLists.svtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
