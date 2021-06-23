svtk_module_find_package(PACKAGE GDAL)

set(classes
  svtkGDAL
  svtkGDALVectorReader
  svtkGDALRasterReader)

svtk_module_add_module(SVTK::IOGDAL
  CLASSES ${classes})
svtk_module_link(SVTK::IOGDAL
  PRIVATE
    GDAL::GDAL)
