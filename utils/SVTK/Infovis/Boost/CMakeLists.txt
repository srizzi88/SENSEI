set(headers
  svtkVariantBoostSerialization.h)

svtk_module_add_module(SVTK::InfovisBoost
  HEADERS ${headers}
  HEADER_ONLY)

svtk_module_find_package(
  PACKAGE     Boost
  COMPONENTS  serialization
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR         "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
svtk_module_link(SVTK::InfovisBoost
  INTERFACE
    Boost::serialization)
