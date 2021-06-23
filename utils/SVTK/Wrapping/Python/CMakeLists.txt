if (NOT DEFINED SVTK_INSTALL_PYTHON_EXES)
  option(SVTK_INSTALL_PYTHON_EXES "Install svtkpython and psvtkpython" ON)
  mark_as_advanced(SVTK_INSTALL_PYTHON_EXES)
endif ()

# Set up rpaths
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX)
  file(RELATIVE_PATH svtkpython_relpath
    "/prefix/${CMAKE_INSTALL_BINDIR}"
    "/prefix/${CMAKE_INSTALL_LIBDIR}")
  if (APPLE)
    set(svtkpython_rpath_prefix
      "@executable_path")
  else ()
    set(svtkpython_rpath_prefix
      "$ORIGIN")
  endif ()

  list(APPEND CMAKE_INSTALL_RPATH
    "${svtkpython_rpath_prefix}/${svtkpython_relpath}")
endif ()

# The interpreters are not supported in wheel builds, so skip them.
if (NOT SVTK_WHEEL_BUILD)
  add_executable(svtkpython
    svtkpython.rc
    svtkPythonAppInit.cxx)
  target_link_libraries(svtkpython
    PRIVATE
      SVTK::WrappingPythonCore
      SVTK::PythonInterpreter
      SVTK::Python
      SVTK::svtkpythonmodules
      SVTK::svtksys)
  add_executable(SVTK::svtkpython ALIAS svtkpython)
  if (SVTK_INSTALL_PYTHON_EXES)
    install(
      TARGETS     svtkpython
      EXPORT      SVTKPython
      DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()

  if (TARGET SVTK::ParallelMPI)
    add_executable(psvtkpython
      svtkPythonAppInit.cxx)
    target_compile_definitions(psvtkpython
      PRIVATE
        SVTK_COMPILED_USING_MPI)
    target_link_libraries(psvtkpython
      PRIVATE
        SVTK::WrappingPythonCore
        SVTK::PythonInterpreter
        SVTK::ParallelMPI
        SVTK::Python
        SVTK::mpi
        SVTK::svtkpythonmodules)
    add_executable(SVTK::psvtkpython ALIAS psvtkpython)
    if (SVTK_INSTALL_PYTHON_EXES)
      install(
        TARGETS     psvtkpython
        EXPORT      SVTKPython
        DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif ()
  endif ()
endif ()

set(_svtk_python_imports)
foreach (_svtk_python_module IN LISTS svtk_python_wrapped_modules)
  get_property(_svtk_python_library_name
    TARGET    "${_svtk_python_module}"
    PROPERTY  "INTERFACE_svtk_module_library_name")
  string(APPEND _svtk_python_imports
    "from .${_svtk_python_library_name} import *\n")
endforeach ()

set(_svtk_python_files)
foreach (_svtk_module IN LISTS svtk_modules)
  _svtk_module_get_module_property("${_svtk_module}"
    PROPERTY "python_modules"
    VARIABLE _svtk_module_python_files)
  list(APPEND _svtk_python_files
    ${_svtk_module_python_files})
endforeach()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkmodules/all.py.in"
  "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/svtkmodules/all.py"
  @ONLY)
list(APPEND _svtk_python_files
  "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/svtkmodules/all.py")

if (BUILD_SHARED_LIBS)
  install(
    FILES       "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/svtkmodules/all.py"
    DESTINATION "${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/svtkmodules")
endif ()

set(python_files
  svtk.py
  svtkmodules/__init__.py
  svtkmodules/gtk/GtkGLExtSVTKRenderWindow.py
  svtkmodules/gtk/GtkGLExtSVTKRenderWindowInteractor.py
  svtkmodules/gtk/GtkSVTKRenderWindow.py
  svtkmodules/gtk/GtkSVTKRenderWindowInteractor.py
  svtkmodules/gtk/__init__.py
  svtkmodules/numpy_interface/__init__.py
  svtkmodules/numpy_interface/algorithms.py
  svtkmodules/numpy_interface/dataset_adapter.py
  svtkmodules/numpy_interface/internal_algorithms.py
  svtkmodules/qt/QSVTKRenderWindowInteractor.py
  svtkmodules/qt/__init__.py
  svtkmodules/test/BlackBox.py
  svtkmodules/test/Testing.py
  svtkmodules/test/__init__.py
  svtkmodules/tk/__init__.py
  svtkmodules/tk/svtkLoadPythonTkWidgets.py
  svtkmodules/tk/svtkTkImageViewerWidget.py
  svtkmodules/tk/svtkTkPhotoImage.py
  svtkmodules/tk/svtkTkRenderWidget.py
  svtkmodules/tk/svtkTkRenderWindowInteractor.py
  svtkmodules/util/__init__.py
  svtkmodules/util/colors.py
  svtkmodules/util/keys.py
  svtkmodules/util/misc.py
  svtkmodules/util/numpy_support.py
  svtkmodules/util/svtkAlgorithm.py
  svtkmodules/util/svtkConstants.py
  svtkmodules/util/svtkImageExportToArray.py
  svtkmodules/util/svtkImageImportFromArray.py
  svtkmodules/util/svtkMethodParser.py
  svtkmodules/util/svtkVariant.py
  svtkmodules/wx/__init__.py
  svtkmodules/wx/wxSVTKRenderWindow.py
  svtkmodules/wx/wxSVTKRenderWindowInteractor.py)

set(python_copied_modules)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file}")
  add_custom_command(
    OUTPUT  "${output_python_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
            "${output_python_file}"
    COMMENT "Copying ${python_file} to the binary directory")
  if (BUILD_SHARED_LIBS)
    get_filename_component(python_file_directory "${python_file}" DIRECTORY)
    install(
      FILES       "${python_file}"
      DESTINATION "${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/${python_file_directory}"
      COMPONENT   "python")
  endif ()
  list(APPEND python_copied_modules
    "${output_python_file}")
endforeach ()

list(APPEND _svtk_python_files
  ${python_copied_modules})

set(_svtk_python_zip)
if (NOT BUILD_SHARED_LIBS)
  set(_svtk_python_zip "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/_svtk.zip")
  add_custom_command(
    OUTPUT            "${_svtk_python_zip}"
    COMMAND           ${CMAKE_COMMAND} -E tar "cfv"
                      "${_svtk_python_zip}" --format=zip ${_svtk_python_files}
    DEPENDS           ${_svtk_python_files}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}"
    COMMENT           "Creating _svtk.zip Python modules archive")
  install(
    FILES       "${_svtk_python_zip}"
    DESTINATION "${SVTK_PYTHON_SITE_PACKAGES_SUFFIX}/"
    COMPONENT   "python")
endif ()

add_custom_target(svtk_python_copy ALL
  DEPENDS
    ${python_copied_modules}
    ${_svtk_python_zip})
