##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#unconditionally include rendering contexts so that the helper functions
#are around even if rendering isn't enabled
include(SVTKmRenderingContexts)

if(NOT SVTKm_ENABLE_RENDERING)
  return()
endif()

# determine what context(s) for rendering we want to build
svtkm_option(SVTKm_ENABLE_GL_CONTEXT "Enable GL context for svtkm rendering" OFF)
if(UNIX AND NOT APPLE)
  svtkm_option(SVTKm_ENABLE_OSMESA_CONTEXT "Enable OSMesa context for svtkm rendering" OFF)
  svtkm_option(SVTKm_ENABLE_EGL_CONTEXT "Enable EGL context for svtkm rendering" OFF)
  mark_as_advanced(SVTKm_ENABLE_OSMESA_CONTEXT)
  mark_as_advanced(SVTKm_ENABLE_EGL_CONTEXT)
endif()
if(SVTKm_ENABLE_GL_CONTEXT AND SVTKm_ENABLE_OSMESA_CONTEXT)
  message(FATAL_ERROR "SVTK-m GL and OSMesa contexts are mutually exclusive")
endif()
if(SVTKm_ENABLE_GL_CONTEXT AND SVTKm_ENABLE_EGL_CONTEXT)
  message(FATAL_ERROR "SVTK-m GL and EGL contexts are mutually exclusive")
endif()
if(SVTKm_ENABLE_EGL_CONTEXT AND SVTKm_ENABLE_OSMESA_CONTEXT)
  message(FATAL_ERROR "SVTK-m EGL and OSMesa contexts are mutually exclusive")
endif()

set(headers
  Actor.h
  AxisAnnotation.h
  AxisAnnotation2D.h
  AxisAnnotation3D.h
  BitmapFont.h
  BitmapFontFactory.h
  BoundingBoxAnnotation.h
  Camera.h
  Canvas.h
  CanvasRayTracer.h
  Color.h
  ColorBarAnnotation.h
  ColorLegendAnnotation.h
  ConnectivityProxy.h
  Cylinderizer.h
  DecodePNG.h
  LineRenderer.h
  MatrixHelpers.h
  Scene.h
  Mapper.h
  MapperCylinder.h
  MapperPoint.h
  MapperQuad.h
  MapperRayTracer.h
  MapperVolume.h
  MapperConnectivity.h
  MapperWireframer.h
  Quadralizer.h
  TextAnnotation.h
  TextAnnotationBillboard.h
  TextAnnotationScreen.h
  TextRenderer.h
  Texture2D.h
  Triangulator.h
  View.h
  View1D.h
  View2D.h
  View3D.h
  Wireframer.h
  WorldAnnotator.h
  )

set(sources
  BitmapFont.cxx
  BitmapFontFactory.cxx

  Camera.cxx
  Color.cxx
  DecodePNG.cxx
  raytracing/Logger.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  Actor.cxx
  AxisAnnotation.cxx
  AxisAnnotation2D.cxx
  AxisAnnotation3D.cxx
  BoundingBoxAnnotation.cxx
  Canvas.cxx
  CanvasRayTracer.cxx
  ColorBarAnnotation.cxx
  ColorLegendAnnotation.cxx
  ConnectivityProxy.cxx
  LineRenderer.cxx
  Mapper.cxx
  MapperConnectivity.cxx
  MapperCylinder.cxx
  MapperPoint.cxx
  MapperQuad.cxx
  MapperRayTracer.cxx
  MapperVolume.cxx
  MapperWireframer.cxx
  Scene.cxx
  TextAnnotation.cxx
  TextAnnotationBillboard.cxx
  TextAnnotationScreen.cxx
  TextRenderer.cxx
  View.cxx
  View1D.cxx
  View2D.cxx
  View3D.cxx
  WorldAnnotator.cxx

  internal/RunTriangulator.cxx
  raytracing/BoundingVolumeHierarchy.cxx
  raytracing/Camera.cxx
  raytracing/ChannelBuffer.cxx
  raytracing/ConnectivityTracer.cxx
  raytracing/CylinderExtractor.cxx
  raytracing/CylinderIntersector.cxx
  raytracing/MeshConnectivityBuilder.cxx
  raytracing/MeshConnectivityContainers.cxx
  raytracing/QuadExtractor.cxx
  raytracing/QuadIntersector.cxx
  raytracing/RayTracer.cxx
  raytracing/RayOperations.cxx
  raytracing/ShapeIntersector.cxx
  raytracing/SphereExtractor.cxx
  raytracing/SphereIntersector.cxx
  raytracing/TriangleExtractor.cxx
  raytracing/TriangleIntersector.cxx
  raytracing/VolumeRendererStructured.cxx
  )

# Note that EGL and OSMesa Canvas depend on the GL version being built. Only
# the None backend supports not building the opengl version
set(opengl_headers
  CanvasGL.h
  MapperGL.h
  TextureGL.h
  WorldAnnotatorGL.h
  )

set(opengl_sources
  CanvasGL.cxx
  MapperGL.cxx
  TextureGL.cxx
  WorldAnnotatorGL.cxx
  )

set(egl_headers
  CanvasEGL.h
)

set(egl_sources
  CanvasEGL.cxx
  )

set(osmesa_headers
  CanvasOSMesa.h
  )

set(osmesa_sources
  CanvasOSMesa.cxx
  )

#-----------------------------------------------------------------------------
svtkm_library(
  NAME svtkm_rendering
  SOURCES ${sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${device_sources}
  )

# Install all headers no matter what backend was selected
svtkm_declare_headers(${opengl_headers})
svtkm_declare_headers(${osmesa_headers})
svtkm_declare_headers(${egl_headers})

if(SVTKm_ENABLE_GL_CONTEXT)
  target_sources(svtkm_rendering PRIVATE ${opengl_sources})
elseif(SVTKm_ENABLE_OSMESA_CONTEXT)
  target_sources(svtkm_rendering PRIVATE ${opengl_sources} ${osmesa_sources})
elseif(SVTKm_ENABLE_EGL_CONTEXT)
  target_sources(svtkm_rendering PRIVATE ${opengl_sources} ${egl_sources})
endif()

#-----------------------------------------------------------------------------
target_link_libraries(svtkm_rendering PUBLIC svtkm_filter
                                     PRIVATE svtkm_lodepng)
if(UNIX AND NOT APPLE)
  target_link_libraries(svtkm_rendering PRIVATE rt)
endif()

#-----------------------------------------------------------------------------
target_link_libraries(svtkm_rendering PUBLIC svtkm_rendering_gl_context)

if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(TARGETS svtkm_rendering_gl_context
    EXPORT ${SVTKm_EXPORT_NAME}
    )
endif()

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(raytracing)
add_subdirectory(testing)
