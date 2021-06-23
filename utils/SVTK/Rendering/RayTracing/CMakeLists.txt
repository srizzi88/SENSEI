set(classes
  svtkOSPRayActorNode
  svtkOSPRayAMRVolumeMapperNode
  svtkOSPRayCameraNode
  svtkOSPRayCompositePolyDataMapper2Node
  svtkOSPRayLightNode
  svtkOSPRayMaterialHelpers
  svtkOSPRayMaterialLibrary
  svtkOSPRayMoleculeMapperNode
  svtkOSPRayPass
  svtkOSPRayPolyDataMapperNode
  svtkOSPRayRendererNode
  svtkOSPRayTetrahedraMapperNode
  svtkOSPRayViewNodeFactory
  svtkOSPRayVolumeMapper
  svtkOSPRayVolumeMapperNode
  svtkOSPRayVolumeNode
  svtkOSPRayWindowNode)

set(sources
    RTWrapper/RTWrapper.cxx)

set(headers
    RTWrapper/RTWrapper.h)

svtk_object_factory_declare(
  BASE svtkOSPRayVolumeInterface
  OVERRIDE svtkOSPRayVolumeMapper)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGRAYTRACING_EXPORT")

option(SVTK_ENABLE_OSPRAY "Enable OSPRay RayTracing backend" ON)
cmake_dependent_option(SVTKOSPRAY_ENABLE_DENOISER
  "build OSPRay Renderer using OpenImageDenoise" OFF
  "SVTK_ENABLE_OSPRAY" OFF)

option(SVTK_ENABLE_VISRTX "Enable VisRTX RayTracing backend" OFF)
if (SVTK_ENABLE_VISRTX)
  list(APPEND sources RTWrapper/VisRTX/VisRTXBackend.cxx)
endif ()

svtk_module_add_module(SVTK::RenderingRayTracing
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source} ${sources}
  PRIVATE_HEADERS ${svtk_object_factory_header} ${headers})

svtk_module_install_headers(
  FILES
    RTWrapper/RTWrapper.h
    RTWrapper/Types.h
    RTWrapper/Backend.h
  SUBDIR "RTWrapper")

if (SVTK_ENABLE_OSPRAY)
  svtk_module_find_package(
    PACKAGE ospray
    VERSION 1.8)

  svtk_module_link(SVTK::RenderingRayTracing
    PUBLIC
      ospray::ospray)
  if (SVTKOSPRAY_ENABLE_DENOISER)
    svtk_module_find_package(
      PACKAGE OpenImageDenoise
      FORWARD_VERSION_REQ MINOR)
    svtk_module_definitions(SVTK::RenderingRayTracing
      PRIVATE
        SVTKOSPRAY_ENABLE_DENOISER)
    svtk_module_link(SVTK::RenderingRayTracing
      PRIVATE
        OpenImageDenoise)
  endif ()

  # TODO: FindOSPRay should do this.
  # OSPRay_Core uses MMTime which is in it's own special library.
  if (WIN32)
    svtk_module_link(SVTK::RenderingRayTracing
      PRIVATE
        Winmm)
  endif ()
  svtk_module_definitions(SVTK::RenderingRayTracing
    PRIVATE
      SVTK_ENABLE_OSPRAY)
endif ()

# VisRTX
if (SVTK_ENABLE_VISRTX)
  svtk_module_find_package(
    PACKAGE VisRTX
    CONFIG_MODE)
  svtk_module_link(SVTK::RenderingRayTracing
    PUBLIC
      VisRTX_DynLoad)
  svtk_module_definitions(SVTK::RenderingRayTracing
    PRIVATE
      SVTK_ENABLE_VISRTX)
endif ()
