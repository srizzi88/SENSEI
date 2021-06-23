#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="png"
readonly ownership="Libpng Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/svtk$name"
readonly repo="https://gitlab.kitware.com/third-party/png.git"
readonly tag="for/svtk-20190605-1.6.37"
readonly paths="
.gitattributes
CMakeLists.svtk.txt

scripts/dfn.awk
scripts/options.awk
scripts/pnglibconf.dfa
scripts/pnglibconf.mak
pngusr.dfa

CHANGES
LICENSE
README
README.kitware.md

png.c
pngerror.c
pngget.c
pngmem.c
pngpread.c
pngread.c
pngrio.c
pngrtran.c
pngrutil.c
pngset.c
pngtrans.c
pngwio.c
pngwrite.c
pngwtran.c
pngwutil.c

png.h
pngconf.h
pngdebug.h
pnginfo.h
pngpriv.h
pngstruct.h
svtk_png_mangle.h
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mkdir -p "svtkzlib"
    sed -e 's/#cmakedefine/#undef/' \
        "${BASH_SOURCE%/*}/../zlib/svtkzlib/zconf.h.cmakein" \
        > svtkzlib/zconf.h
    CPPFLAGS="-I${BASH_SOURCE%/*}/../zlib/svtkzlib -I$PWD -DZ_PREFIX -DZ_HAVE_UNISTD_H" make -f scripts/pnglibconf.mak
    rm -rvf "svtkzlib"
    sed -i -e '/PNG_ZLIB_VERNUM/s/0x.*/0/' pnglibconf.h
    rm -rvf scripts pngusr.dfa pnglibconf.dfn pnglibconf.pre pnglibconf.out
    mv -v CMakeLists.svtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
