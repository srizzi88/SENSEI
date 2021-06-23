svtk_module_find_package(PACKAGE PDAL)

set(classes
  svtkPDALReader)

svtk_module_add_module(SVTK::IOPDAL
  CLASSES ${classes})
svtk_module_link(SVTK::IOPDAL
  PRIVATE
    pdal_base)
# XXX(pdal): PDAL should add include interfaces to its exported targets. See
# https://github.com/PDAL/PDAL/issues/2262.
svtk_module_include(SVTK::IOPDAL
  PRIVATE
    ${PDAL_INCLUDE_DIRS})
