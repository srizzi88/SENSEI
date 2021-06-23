#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lcl"
readonly ownership="Lightweight Cell Library Upstream <kwrobot@kitware.com>"
readonly subtree="svtkm/thirdparty/$name/svtkm$name"
readonly repo="https://gitlab.kitware.com/svtk/lcl.git"
readonly tag="master"
readonly paths="
lcl
LICENSE.md
README.md
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    rm -rf lcl/testing
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
