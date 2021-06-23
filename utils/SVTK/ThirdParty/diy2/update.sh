#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="diy2"
readonly ownership="Diy2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/diy2.git"
readonly tag="for/svtk-20191127-gd6d04d0d"
readonly paths="
.gitattributes
CMakeLists.svtk.txt
include
LEGAL.txt
LICENSE.txt
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv include/diy include/svtkdiy2
    mv CMakeLists.svtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
