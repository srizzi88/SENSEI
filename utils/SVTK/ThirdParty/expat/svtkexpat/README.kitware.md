# Expat fork for SVTK

This branch contains changes required to embed Expat into SVTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Add attributes to pass commit checks within SVTK.
  * Add CMake code to integrate with SVTK's module system.
  * Mangle all exported symbols to have a `svtkexpat_` prefix.
