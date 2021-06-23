set(classes
  svtkCameraPass
  svtkClearRGBPass
  svtkClearZPass
  svtkCompositePolyDataMapper2
  svtkDefaultPass
  svtkDepthImageProcessingPass
  svtkDepthOfFieldPass
  svtkDepthPeelingPass
  svtkDualDepthPeelingPass
  svtkEDLShading
  svtkEquirectangularToCubeMapTexture
  svtkFramebufferPass
  svtkGaussianBlurPass
  svtkGenericOpenGLRenderWindow
  svtkHiddenLineRemovalPass
  svtkImageProcessingPass
  svtkLightingMapPass
  svtkLightsPass
  svtkOpaquePass
  svtkOpenGLActor
  svtkOpenGLBillboardTextActor3D
  svtkOpenGLBufferObject
  svtkOpenGLCamera
  svtkOpenGLCellToSVTKCellMap
  svtkOpenGLFXAAFilter
  svtkOpenGLFXAAPass
  svtkOpenGLFluidMapper
  svtkOpenGLFramebufferObject
  svtkOpenGLGL2PSHelper
  svtkOpenGLGlyph3DHelper
  svtkOpenGLGlyph3DMapper
  svtkOpenGLHardwareSelector
  svtkOpenGLHelper
  svtkOpenGLImageAlgorithmHelper
  svtkOpenGLImageMapper
  svtkOpenGLImageSliceMapper
  svtkOpenGLIndexBufferObject
  svtkOpenGLInstanceCulling
  svtkOpenGLLabeledContourMapper
  svtkOpenGLLight
  svtkOpenGLPointGaussianMapper
  svtkOpenGLPolyDataMapper
  svtkOpenGLPolyDataMapper2D
  svtkOpenGLProperty
  svtkOpenGLQuadHelper
  svtkOpenGLRenderPass
  svtkOpenGLRenderTimer
  svtkOpenGLRenderTimerLog
  svtkOpenGLRenderUtilities
  svtkOpenGLRenderWindow
  svtkOpenGLRenderer
  svtkOpenGLShaderCache
  svtkOpenGLShaderProperty
  svtkOpenGLSkybox
  svtkOpenGLSphereMapper
  svtkOpenGLState
  svtkOpenGLStickMapper
  svtkOpenGLTextActor
  svtkOpenGLTextActor3D
  svtkOpenGLTextMapper
  svtkOpenGLTexture
  svtkOpenGLUniforms
  svtkOpenGLVertexArrayObject
  svtkOpenGLVertexBufferObject
  svtkOpenGLVertexBufferObjectCache
  svtkOpenGLVertexBufferObjectGroup
  svtkOrderIndependentTranslucentPass
  svtkOverlayPass
  svtkPanoramicProjectionPass
  svtkPixelBufferObject
  svtkPointFillPass
  svtkPBRIrradianceTexture
  svtkPBRLUTTexture
  svtkPBRPrefilterTexture
  svtkRenderPassCollection
  svtkRenderStepsPass
  svtkRenderbuffer
  svtkSSAAPass
  svtkSSAOPass
  svtkSequencePass
  svtkShader
  svtkShaderProgram
  svtkShadowMapBakerPass
  svtkShadowMapPass
  svtkSimpleMotionBlurPass
  svtkSobelGradientMagnitudePass
  svtkTextureObject
  svtkTextureUnitManager
  svtkToneMappingPass
  svtkTransformFeedback
  svtkTranslucentPass
  svtkValuePass
  svtkVolumetricPass)

if (NOT DEFINED OPENGL_ES_VERSION)
  list(APPEND classes
    svtkDataTransferHelper)
endif()

set(headers
  svtkCompositePolyDataMapper2Internal.h
  svtkOpenGL.h
  svtkStateStorage.h
  "${CMAKE_CURRENT_BINARY_DIR}/svtkOpenGLError.h"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkRenderingOpenGLConfigure.h")

# add opengl support test execulatble on windows
if (WIN32 AND (NOT CMAKE_CROSSCOMPILING OR DEFINED CMAKE_CROSSCOMPILING_EMULATOR))
  svtk_module_add_executable(svtkTestOpenGLVersion svtkTestOpenGLVersion.cxx)
  set_target_properties(svtkTestOpenGLVersion
    PROPERTIES
      WIN32_EXECUTABLE TRUE)
  target_link_libraries(svtkTestOpenGLVersion PRIVATE opengl32)
  list(APPEND headers svtkTestOpenGLVersion.h)
endif ()

set(sources)
set(private_headers)

set(shader_files
  glsl/svtkDepthOfFieldPassFS.glsl
  glsl/svtkDepthPeelingPassFinalFS.glsl
  glsl/svtkDepthPeelingPassIntermediateFS.glsl
  glsl/svtkEDLBilateralFilterFS.glsl
  glsl/svtkEDLComposeFS.glsl
  glsl/svtkEDLShadeFS.glsl
  glsl/svtkFXAAFilterFS.glsl
  glsl/svtkFluidMapperDepthFilterBiGaussFS.glsl
  glsl/svtkFluidMapperDepthFilterNarrowRangeFS.glsl
  glsl/svtkFluidMapperSurfaceNormalFS.glsl
  glsl/svtkFluidMapperThicknessAndVolumeColorFilterFS.glsl
  glsl/svtkFluidMapperFS.glsl
  glsl/svtkFluidMapperFinalFS.glsl
  glsl/svtkFluidMapperGS.glsl
  glsl/svtkFluidMapperVS.glsl
  glsl/svtkGaussianBlurPassFS.glsl
  glsl/svtkGaussianBlurPassVS.glsl
  glsl/svtkGlyph3DVS.glsl
  glsl/svtkOrderIndependentTranslucentPassFinalFS.glsl
  glsl/svtkPointFillPassFS.glsl
  glsl/svtkPointGaussianVS.glsl
  glsl/svtkPointGaussianGS.glsl
  glsl/svtkPolyData2DFS.glsl
  glsl/svtkPolyData2DVS.glsl
  glsl/svtkPolyDataFS.glsl
  glsl/svtkPolyDataVS.glsl
  glsl/svtkPolyDataWideLineGS.glsl
  glsl/svtkSSAAPassFS.glsl
  glsl/svtkSimpleMotionBlurPassFS.glsl
  glsl/svtkSobelGradientMagnitudePass1FS.glsl
  glsl/svtkSobelGradientMagnitudePass2FS.glsl
  glsl/svtkSphereMapperGS.glsl
  glsl/svtkStickMapperGS.glsl
  glsl/svtkTextureObjectFS.glsl
  glsl/svtkTextureObjectVS.glsl)
unset(shader_h_files)
foreach(file IN LISTS shader_files)
  svtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "SVTKRENDERINGOPENGL2_EXPORT"
    EXPORT_HEADER "svtkRenderingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    "${source}")
  list(APPEND private_headers
    "${header}")
endforeach()

# setup factory overrides
set(opengl_overrides
  Actor
  BillboardTextActor3D
  Camera
  LabeledContourMapper
  HardwareSelector
  ImageMapper
  ImageSliceMapper
  Glyph3DMapper
  Light
  PointGaussianMapper
  PolyDataMapper
  PolyDataMapper2D
  Property
  ShaderProperty
  Uniforms
  Renderer
  RenderTimerLog
  Skybox
  TextActor
  TextActor3D
  TextMapper
  Texture)

# Now to see about the GPU info list.
list(APPEND classes svtkDummyGPUInfoList)
svtk_object_factory_declare(
  BASE svtkGPUInfoList
  OVERRIDE svtkDummyGPUInfoList)

foreach (opengl_override IN LISTS opengl_overrides)
  svtk_object_factory_declare(
    BASE "svtk${opengl_override}"
    OVERRIDE "svtkOpenGL${opengl_override}")
endforeach ()

# Update sources to include render window specializations for
# offscreen-only modes, if enabled.
if (SVTK_OPENGL_HAS_OSMESA)
  list(APPEND classes svtkOSOpenGLRenderWindow)
endif ()
if (SVTK_OPENGL_HAS_EGL)
  list(APPEND classes svtkEGLRenderWindow)
  set_source_files_properties(svtkEGLRenderWindow.cxx
    PROPERTIES
      COMPILE_DEFINITIONS "SVTK_DEFAULT_EGL_DEVICE_INDEX=${SVTK_DEFAULT_EGL_DEVICE_INDEX}")
endif ()

set(has_svtkRenderWindow_override 0)
# Setup factory overrides and add window system specific render windows.
if (SVTK_USE_X)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindow
      OVERRIDE svtkXOpenGLRenderWindow)
    set(has_svtkRenderWindow_override 1)
  endif ()
  list(APPEND classes svtkXOpenGLRenderWindow)
elseif (WIN32)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindow
      OVERRIDE svtkWin32OpenGLRenderWindow)
    set(has_svtkRenderWindow_override 1)
  endif ()
  list(APPEND classes svtkWin32OpenGLRenderWindow)
elseif (SVTK_USE_COCOA)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindow
      OVERRIDE svtkCocoaRenderWindow)
    set(has_svtkRenderWindow_override 1)
  endif ()
  list(APPEND sources
    svtkCocoaRenderWindow.mm
    svtkCocoaGLView.mm)
  list(APPEND headers
    svtkCocoaRenderWindow.h
    svtkCocoaGLView.h)
  # Set specified Objective-C++ flags, if any.
  if (SVTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      svtkCocoaRenderWindow.mm
      svtkCocoaGLView.mm
      PROPERTIES
        COMPILE_FLAGS "${SVTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
elseif (ANDROID)
  # Add some custom overrides
  svtk_object_factory_declare(
    BASE svtkRenderWindow
    OVERRIDE svtkEGLRenderWindow)
  set(has_svtkRenderWindow_override 1)
  include_directories("${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
elseif (APPLE_IOS)
  # Add some custom overrides
  svtk_object_factory_declare(
    BASE svtkRenderWindow
    OVERRIDE svtkIOSRenderWindow)
  set(has_svtkRenderWindow_override 1)
  list(APPEND sources svtkIOSRenderWindow.mm)
  list(APPEND headers svtkIOSRenderWindow.h)
  # Set specified Objective-C++ flags, if any.
  if (SVTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      svtkIOSRenderWindow.mm
      PROPERTIES
        COMPILE_FLAGS "${SVTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
endif ()

if (SVTK_DEFAULT_RENDER_WINDOW_HEADLESS OR NOT has_svtkRenderWindow_override)
  if (SVTK_OPENGL_HAS_EGL)
    svtk_object_factory_declare(
      BASE svtkRenderWindow
      OVERRIDE svtkEGLRenderWindow)
  elseif (SVTK_OPENGL_HAS_OSMESA)
    svtk_object_factory_declare(
      BASE svtkRenderWindow
      OVERRIDE svtkOSOpenGLRenderWindow)
  endif ()
endif ()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGOPENGL2_EXPORT")

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect SVTK's OpenGL errors and vice-versa.
set(SVTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
cmake_dependent_option(SVTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS
  "Enable OpenGL error check and reporting in non-debug builds." OFF
  SVTK_REPORT_OPENGL_ERRORS OFF)
mark_as_advanced(SVTK_REPORT_OPENGL_ERRORS)
mark_as_advanced(SVTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkOpenGLError.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkOpenGLError.h")

# Add OpenGL command stream annotations via
# svtkOpenGLRenderUtilities::MarkDebugEvent when enabled:
option(SVTK_OPENGL_ENABLE_STREAM_ANNOTATIONS
  "Enable debugging annotations in the OpenGL command stream."
  OFF
)
mark_as_advanced(SVTK_OPENGL_ENABLE_STREAM_ANNOTATIONS)

# Configure the module specific settings into a module configured header.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkRenderingOpenGLConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkRenderingOpenGLConfigure.h")

svtk_module_add_module(SVTK::RenderingOpenGL2
  CLASSES ${classes}
  SOURCES ${sources} ${svtk_object_factory_source}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers} ${svtk_object_factory_header})

if (SVTK_USE_X)
  svtk_module_find_package(
    PACKAGE X11)
  svtk_module_link(SVTK::RenderingOpenGL2
    PUBLIC
      X11::X11)
elseif (SVTK_USE_COCOA)
  svtk_module_link(SVTK::RenderingOpenGL2 PUBLIC "-framework Cocoa")
endif ()

if (ANDROID)
  svtk_module_link(SVTK::RenderingOpenGL2
    PRIVATE
      android)
endif ()

if (APPLE_IOS)
  svtk_module_link(SVTK::RenderingOpenGL2 PUBLIC "-framework UIKit")
endif ()

if (NOT ANDROID AND
    NOT APPLE_IOS AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten" AND
    NOT SVTK_OPENGL_USE_GLES)
  set(probe_no_install)
  if (NOT _svtk_build_INSTALL_HEADERS)
    set(probe_no_install
      NO_INSTALL)
  endif ()
  svtk_module_add_executable(svtkProbeOpenGLVersion
    ${probe_no_install}
    svtkProbeOpenGLVersion.cxx)
  target_link_libraries(svtkProbeOpenGLVersion
    PRIVATE
      SVTK::RenderingOpenGL2)
  svtk_module_autoinit(
    TARGETS svtkProbeOpenGLVersion
    MODULES SVTK::RenderingOpenGL2)
endif ()
