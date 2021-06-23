set(classes
  svtkMultiBlockVolumeMapper
  svtkOpenGLGPUVolumeRayCastMapper
  svtkOpenGLProjectedTetrahedraMapper
  svtkOpenGLRayCastImageDisplayHelper
  svtkOpenGLVolumeGradientOpacityTable
  svtkOpenGLVolumeLookupTable
  svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D
  svtkOpenGLVolumeMaskTransferFunction2D
  svtkOpenGLVolumeOpacityTable
  svtkOpenGLVolumeRGBTable
  svtkOpenGLVolumeTransferFunction2D
  svtkSmartVolumeMapper
  svtkVolumeInputHelper
  svtkVolumeTexture)

set(shader_files
  shaders/raycasterfs.glsl
  shaders/raycastervs.glsl
  shaders/svtkglProjectedTetrahedraVS.glsl
  shaders/svtkglProjectedTetrahedraFS.glsl)

set(template_classes
  svtkOpenGLVolumeLookupTables
)

set(sources)
set(headers)
foreach (shader_file IN LISTS shader_files)
  svtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "SVTKRENDERINGVOLUMEOPENGL2_EXPORT"
    EXPORT_HEADER "svtkRenderingVolumeOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND header ${header})
endforeach ()

# Now for the object factory.
set(opengl_overrides
  GPUVolumeRayCastMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper)

foreach (opengl_override IN LISTS opengl_overrides)
  svtk_object_factory_declare(
    BASE "svtk${opengl_override}"
    OVERRIDE "svtkOpenGL${opengl_override}")
endforeach ()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGVOLUMEOPENGL2_EXPORT")

svtk_module_add_module(SVTK::RenderingVolumeOpenGL2
  CLASSES ${classes}
  TEMPLATE_CLASSES  ${template_classes}
  SOURCES ${sources} ${svtk_object_factory_source}
  PRIVATE_HEADERS ${headers} ${svtk_object_factory_header})
