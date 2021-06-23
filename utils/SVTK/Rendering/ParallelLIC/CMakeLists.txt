set(classes
  svtkMPIPixelTT
  svtkParallelTimer
  svtkPLineIntegralConvolution2D
  svtkPPainterCommunicator
  svtkPPixelTransfer
  svtkPSurfaceLICComposite
  svtkPSurfaceLICInterface)

svtk_object_factory_declare(
  BASE svtkSurfaceLICInterface
  OVERRIDE svtkPSurfaceLICInterface)
svtk_object_factory_declare(
  BASE svtkLineIntegralConvolution2D
  OVERRIDE svtkPLineIntegralConvolution2D)
svtk_object_factory_declare(
  BASE svtkSurfaceLICComposite
  OVERRIDE svtkPSurfaceLICComposite)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGPARALLELLIC_EXPORT")

set(shader_files
  svtkPSurfaceLICComposite_CompFS.glsl)

set(sources)
set(headers)
foreach(shader_file IN LISTS shader_files)
  svtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "SVTKRENDERINGPARALLELLIC_EXPORT"
    EXPORT_HEADER "svtkRenderingParallelLICModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND headers ${header})
endforeach()

svtk_module_add_module(SVTK::RenderingParallelLIC
  CLASSES ${classes}
  SOURCES ${sources} ${svtk_object_factory_source}
  PRIVATE_HEADERS ${headers} ${svtk_object_factory_header})

option(SVTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER "enable parallel timers for the surface lic painter" OFF)
mark_as_advanced(SVTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
svtk_deprecated_setting(convolution_default
  SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVOLUTION2D_TIMER
  SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER
  OFF)
option(SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVOLUTION2D_TIMER "enable parallel timers for the 2d line integral convolution" "${convolution_default}")
mark_as_advanced(SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVOLUTION2D_TIMER)

if (SVTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
  svtk_module_definitions(SVTK::RenderingParallelLIC
    PRIVATE
      svtkSurfaceLICPainterTIME)
endif()
if (SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVOLUTION2D_TIMER)
  svtk_module_definitions(SVTK::RenderingParallelLIC
    PRIVATE
      svtkLineIntegralConvolution2DTIME)
endif()
