#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="diy"
readonly ownership="Diy Upstream <kwrobot@kitware.com>"
readonly subtree="svtkm/thirdparty/$name/svtkm$name"
readonly repo="https://gitlab.kitware.com/third-party/diy2.git"
readonly tag="for/svtk-m"
readonly paths="
include
LEGAL.txt
LICENSE.txt
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv include/diy include/svtkmdiy
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
