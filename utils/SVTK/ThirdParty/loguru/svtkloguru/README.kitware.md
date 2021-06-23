# loguru fork for SVTK

This branch contains chanegs required to embed loguru into SVTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree as well as a header to facilitate mangling of the symbols to
avoid conflicts with other copies of the library within a single process.

  * ignore whitespace issues
  * integreate with SVTK's module system
  * mangle `loguru` namespace to `svtkloguru`
  * add license
