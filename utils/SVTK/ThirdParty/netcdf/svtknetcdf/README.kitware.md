# netcdf fork for SVTK

This branch contains changes required to embed netcdf into SVTK. This
includes changes made primarily to the build system to allow it to be embedded
into another source tree as well as a header to facilitate mangling of the
symbols to avoid conflicts with other copies of the library within a single
process.

  * Ignore whitespace errors to pass SVTK's commit checks.
  * Use SVTK's hdf5 library.
  * Integrate the CMake build with SVTK's module system.
  * Mangle all exported symbols to have a `svtknetcdf_` prefix.
