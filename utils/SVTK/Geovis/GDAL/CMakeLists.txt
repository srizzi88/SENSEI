svtk_module_find_package(PACKAGE GDAL)

set(classes
  svtkGDALRasterConverter
  svtkGDALRasterReprojection
  svtkRasterReprojectionFilter)

svtk_module_add_module(SVTK::GeovisGDAL
  CLASSES ${classes})
svtk_module_link(SVTK::GeovisGDAL
  PRIVATE
    GDAL::GDAL)
