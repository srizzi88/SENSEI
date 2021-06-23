set(classes
  svtkCompositeSurfaceLICMapper
  svtkImageDataLIC2D
  svtkLICNoiseHelper
  svtkLineIntegralConvolution2D
  svtkStructuredGridLIC2D
  svtkSurfaceLICComposite
  svtkSurfaceLICHelper
  svtkSurfaceLICInterface
  svtkSurfaceLICMapper
  svtkTextureIO)

set(headers
  svtkPainterCommunicator.h)

set(shader_files
  svtkLineIntegralConvolution2D_AAH.glsl
  svtkLineIntegralConvolution2D_AAV.glsl
  svtkLineIntegralConvolution2D_CE.glsl
  svtkLineIntegralConvolution2D_EE.glsl
  svtkLineIntegralConvolution2D_LIC0.glsl
  svtkLineIntegralConvolution2D_LICI.glsl
  svtkLineIntegralConvolution2D_LICN.glsl
  svtkLineIntegralConvolution2D_VT.glsl
  svtkStructuredGridLIC2D_fs.glsl
  svtkSurfaceLICInterface_CE.glsl
  svtkSurfaceLICInterface_DCpy.glsl
  svtkSurfaceLICInterface_SC.glsl)

set(shader_h_files)
set(shader_cxx_files)
foreach (shader_file IN LISTS shader_files)
  svtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "SVTKRENDERINGLICOPENGL2_EXPORT"
    EXPORT_HEADER "svtkRenderingLICOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND shader_cxx_files ${source})
  list(APPEND shader_h_files ${header})
endforeach ()

svtk_module_add_module(SVTK::RenderingLICOpenGL2
  CLASSES ${classes}
  SOURCES ${shader_cxx_files}
  HEADERS ${headers}
  PRIVATE_HEADERS ${shader_h_files})

if (SVTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
  svtk_module_definitions(SVTK::RenderingLICOpenGL2
    PRIVATE
      svtkSurfaceLICPainterTIME)
endif ()
if (SVTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVOLUTION2D_TIMER)
  svtk_module_definitions(SVTK::RenderingLICOpenGL2
    PRIVATE
      svtkLineIntegralConvolution2DTIME)
endif ()
