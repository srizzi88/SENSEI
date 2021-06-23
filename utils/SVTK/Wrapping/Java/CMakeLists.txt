set(CMAKE_INSTALL_JNILIBDIR ""
  CACHE PATH "JNI libraries (LIBDIR/java/svtk-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
mark_as_advanced(CMAKE_INSTALL_JNILIBDIR)
if (NOT CMAKE_INSTALL_JNILIBDIR)
  set(CMAKE_INSTALL_JNILIBDIR
    "${CMAKE_INSTALL_LIBDIR}/java/svtk-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif ()
if (IS_ABSOLUTE "${CMAKE_INSTALL_JNILIBDIR}")
  message(FATAL_ERROR
    "The `CMAKE_INSTALL_JNILIBDIR` must not be an absolute path.")
endif ()

include(svtkModuleWrapJava)
svtk_module_wrap_java(
  JAVA_OUTPUT     "${CMAKE_CURRENT_BINARY_DIR}/svtk"
  MODULES         ${svtk_modules}
  WRAPPED_MODULES svtk_java_wrapped_modules
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  # Update svtkSettings.java.in if this changes.
  JNILIB_DESTINATION  "${CMAKE_INSTALL_JNILIBDIR}")

# TODO: Make FindJava.cmake use imported targets.
find_package(Java REQUIRED COMPONENTS Development)
include(UseJava)

option(SVTK_JAVA_SWT_COMPONENT "Enable the SWT component for Java" OFF)
if (SVTK_JAVA_SWT_COMPONENT)
  find_package(SWT REQUIRED)
endif ()

# Add the option for build the JOGL component to SVTK.
option(SVTK_JAVA_JOGL_COMPONENT "Need JOGL jar files" OFF)
if (SVTK_JAVA_JOGL_COMPONENT)
  find_package(JOGL REQUIRED)
endif ()

set(java_sources
  svtk/CellType
  svtk/svtkJavaGarbageCollector
  svtk/svtkJavaMemoryManager
  svtk/svtkJavaMemoryManagerImpl
  svtk/svtkReferenceInformation)

if (TARGET SVTK::RenderingCore)
  list(APPEND java_sources
    svtk/AxesActor)

  if (TARGET SVTK::TestingRendering)
    list(APPEND java_sources
      svtk/svtkJavaTesting)
  endif ()

  if (TARGET SVTK::IOImage)
    list(APPEND java_sources
      svtk/svtkPanel

      svtk/rendering/svtkAbstractComponent
      svtk/rendering/svtkAbstractEventInterceptor
      svtk/rendering/svtkComponent
      svtk/rendering/svtkEventInterceptor
      svtk/rendering/svtkInteractorForwarder

      svtk/rendering/awt/svtkAwtComponent
      svtk/rendering/awt/svtkInternalAwtComponent
      svtk/rendering/awt/svtkPanelComponent

      svtk/sample/Demo
      svtk/sample/InternalFrames
      svtk/sample/ReleaseVtkPanel
      svtk/sample/SimpleSVTK
      svtk/sample/SVTKCanvas

      svtk/sample/rendering/AwtConeRendering
      svtk/sample/rendering/annotation/LabeledCubeAxesActor)

    if (SVTK_JAVA_SWT_COMPONENT)
      list(APPEND java_sources
        svtk/rendering/swt/svtkInternalSwtComponent
        svtk/rendering/swt/svtkSwtComponent
        svtk/rendering/swt/svtkSwtInteractorForwarderDecorator

        svtk/sample/rendering/SwtConeRendering)
    endif ()

    if (SVTK_JAVA_JOGL_COMPONENT)
      list(APPEND java_sources
        svtk/rendering/jogl/svtkAbstractJoglComponent
        svtk/rendering/jogl/svtkJoglCanvasComponent
        svtk/rendering/jogl/svtkJoglPanelComponent

        svtk/sample/rendering/JoglConeRendering)
    endif ()
  endif ()
endif ()

if (TARGET SVTK::InteractionWidgets)
  list(APPEND java_sources
    svtk/svtkCanvas)

  if (TARGET SVTK::RenderingCore)
    list(APPEND java_sources
      svtk/svtkRenderWindowPanel)
  endif ()
endif ()

if (TARGET SVTK::FiltersModeling AND TARGET SVTK::IOImage)
  list(APPEND java_sources
    svtk/sample/ImagePlaneWidget)
endif ()

set(java_input_files)
set(java_class_files)
foreach (java_file IN LISTS java_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java")
  add_custom_command(
    OUTPUT  "${output_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java"
            "${output_file}"
    COMMENT "Copying ${java_file} to the binary directory")

  list(APPEND java_input_files
    "${output_file}")
endforeach ()

set(svtk_java_native_libraries)
set(svtk_java_wrapped_module_libraries)
foreach (_svtk_java_module IN LISTS svtk_java_wrapped_modules)
  get_property(_svtk_java_library_name GLOBAL
    PROPERTY "_svtk_module_${_svtk_java_module}_library_name")
  list(APPEND svtk_java_wrapped_module_libraries
    "${_svtk_java_library_name}")
  get_property(_svtk_module_java_files
    TARGET    "${_svtk_java_library_name}Java"
    PROPERTY  "_svtk_module_java_files")
  list(APPEND java_input_files
    ${_svtk_module_java_files})

  string(APPEND svtk_java_native_libraries
    "  ${_svtk_java_library_name}(\"${_svtk_java_library_name}Java\"),\n")
endforeach ()

# Replace the last comma with a semicolon.
string(REGEX REPLACE ",\n$" ";\n" svtk_java_native_libraries "${svtk_java_native_libraries}")

set(CMAKE_INSTALL_JARDIR ""
  CACHE STRING "Java jar files (LIBDIR/java)")
mark_as_advanced(CMAKE_INSTALL_JARDIR)

# XXX: Handle any cache entries which use `<LIBDIR>` replacements.
if (CMAKE_INSTALL_JARDIR STREQUAL "<LIBDIR>/java")
  # If using the default, blow it away.
  set_property(CACHE CMAKE_INSTALL_JARDIR
    PROPERTY VALUE "")
endif ()
if (CMAKE_INSTALL_JARDIR MATCHES "<LIBDIR>")
  # Replace it in any other usage.
  string(REPLACE "<LIBDIR>" "${CMAKE_INSTALL_LIBDIR}" CMAKE_INSTALL_JARDIR "${CMAKE_INSTALL_JARDIR}")
  set_property(CACHE CMAKE_INSTALL_JARDIR
    PROPERTY VALUE "")
endif ()
# XXX: See above

if (NOT CMAKE_INSTALL_JARDIR)
  set(CMAKE_INSTALL_JARDIR
    "${CMAKE_INSTALL_LIBDIR}/java")
endif ()
if (IS_ABSOLUTE "${CMAKE_INSTALL_JARDIR}")
  message(FATAL_ERROR
    "The `CMAKE_INSTALL_JARDIR` must not be an absolute path.")
endif ()
file(RELATIVE_PATH jnilib_relative_path
  "/prefix/${CMAKE_INSTALL_JARDIR}"
  "/prefix/${CMAKE_INSTALL_JNILIBDIR}")

set(java_configure_sources
  svtk/svtkNativeLibrary
  svtk/svtkSettings)

file(RELATIVE_PATH _svtk_add_executable_relpath
  "/prefix/${_svtk_build_RUNTIME_DESTINATION}"
  "/prefix/${_svtk_build_LIBRARY_DESTINATION}")

foreach (java_file IN LISTS java_configure_sources)
  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${java_file}.java.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${java_file}.java"
    @ONLY)

  list(APPEND java_input_files
    "${output_file}")
endforeach ()

# Set the javac source version
set(SVTK_JAVA_SOURCE_VERSION "<DEFAULT>" CACHE STRING "javac source version")
mark_as_advanced(SVTK_JAVA_SOURCE_VERSION)
set(SVTK_JAVA_TARGET_VERSION "<DEFAULT>" CACHE STRING "javac target version")
mark_as_advanced(SVTK_JAVA_TARGET_VERSION)

if (SVTK_JAVA_SOURCE_VERSION STREQUAL "<DEFAULT>")
  set(SVTK_JAVA_SOURCE_VERSION "1.6")
endif ()
if (SVTK_JAVA_TARGET_VERSION STREQUAL "<DEFAULT>")
  set(SVTK_JAVA_TARGET_VERSION "1.6")
endif ()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_JARDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_JARDIR}")

set(classpath_separator ":")
if (WIN32)
  set(classpath_separator "\\;")
endif ()
set(java_classpath
  "${CMAKE_CURRENT_BINARY_DIR}")
if (SVTK_JAVA_JOGL_COMPONENT)
  list(APPEND java_classpath
    "$<TARGET_PROPERTY:JOGL::glue,LOCATION>"
    "$<TARGET_PROPERTY:JOGL::JOGL,LOCATION>")
endif ()
if (SVTK_JAVA_SWT_COMPONENT)
  list(APPEND java_classpath
    ${SWT_ECLIPSE_LIBRARIES})
endif ()
string(REPLACE ";" "${classpath_separator}" java_classpath "${java_classpath}")

add_library(svtkjava STATIC
  ${java_input_files})
add_library(SVTK::svtkjava ALIAS svtkjava)
set_property(TARGET svtkjava
  PROPERTY
    LINKER_LANGUAGE Java)
set_property(TARGET svtkjava
  PROPERTY
    OUTPUT_NAME "svtk")
target_include_directories(svtkjava
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(svtkjava
  PRIVATE
    # TODO(java): Why does adding this to the include directories not work?
    "SHELL:-classpath \"${java_classpath}\""
    "SHELL:-source ${SVTK_JAVA_SOURCE_VERSION}"
    "SHELL:-target ${SVTK_JAVA_TARGET_VERSION}")
target_link_libraries(svtkjava
  PRIVATE
    $<$<BOOL:${SVTK_JAVA_SWT_COMPONENT}>:${SWT_ECLIPSE_LIBRARIES}>
    "$<$<BOOL:${SVTK_JAVA_JOGL_COMPONENT}>:JOGL::JOGL>")

install(
  TARGETS svtkjava
  EXPORT SVTKJava
  COMPONENT "jar"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_JARDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_JARDIR}")
export(
  EXPORT    SVTKJava
  NAMESPACE SVTK::
  FILE      "${CMAKE_BINARY_DIR}/${svtk_cmake_destination}/SVTKJava-targets.cmake")
install(
  EXPORT    SVTKJava
  NAMESPACE SVTK::
  FILE      "SVTKJava-targets.cmake"
  DESTINATION "${svtk_cmake_destination}"
  COMPONENT "development")

if (BUILD_TESTING AND TARGET SVTK::TestingRendering)
  add_subdirectory(Testing)
endif ()
