#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="doubleconversion"
readonly ownership="double-conversion Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/double-conversion.git"
readonly tag="for/svtk-20191226-3.1.5"
readonly paths="
AUTHORS
Changelog
CMakeLists.svtk.txt
COPYING
double-conversion/
.gitattributes
LICENSE
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.svtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
