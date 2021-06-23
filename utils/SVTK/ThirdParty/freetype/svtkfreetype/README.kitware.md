# freetype fork for SVTK

This branch contains changes required to embed freetype into SVTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add attributes to pass commit checks within SVTK.
  * Use SVTK's zlib library.
  * Integrate CMake code with SVTK's module system.
  * Mangle all exported symbols to have a `svtkfreetype_` prefix.
