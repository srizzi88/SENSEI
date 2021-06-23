#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lodepng"
readonly ownership="LodePNG Upstream <kwrobot@kitware.com>"
readonly subtree="svtkm/thirdparty/$name/svtkm$name"
readonly repo="https://gitlab.kitware.com/third-party/$name.git"
readonly tag="for/svtk-m"
readonly paths="
LICENSE
lodepng.cpp
lodepng.h
README.md
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
