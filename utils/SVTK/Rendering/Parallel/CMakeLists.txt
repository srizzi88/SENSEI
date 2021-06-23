set(classes
  svtkClientServerCompositePass
  svtkClientServerSynchronizedRenderers
  svtkCompositedSynchronizedRenderers
  svtkCompositer
  svtkCompositeRenderManager
  svtkCompositeRGBAPass
  svtkCompositeZPass
  svtkCompressCompositer
  svtkImageRenderManager
  svtkParallelRenderManager
  svtkPHardwareSelector
  svtkSynchronizedRenderers
  svtkSynchronizedRenderWindows
  svtkTreeCompositer)

set(shader_files
  svtkCompositeZPassShader_fs.glsl
  svtkCompositeZPassFS.glsl)

set(sources)
set(private_headers)
foreach (shader_file IN LISTS shader_files)
  svtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "SVTKRENDERINGPARALLEL_EXPORT"
    EXPORT_HEADER "svtkRenderingParallelModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()

svtk_module_add_module(SVTK::RenderingParallel
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

svtk_module_definitions(SVTK::RenderingParallel
  PRIVATE
    SVTK_OPENGL2)
