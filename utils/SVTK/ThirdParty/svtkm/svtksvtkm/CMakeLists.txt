if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/svtk-m/CMakeLists.txt")
  message(FATAL_ERROR
    "SVTKm requested, but the SVTKm submodule is not initialized. Please run "
    "'git submodule update --init --recursive' in the source directory.")
endif ()

if (SVTK_SMP_IMPLEMENTATION_TYPE STREQUAL "OpenMP")
  set(SVTKm_ENABLE_OPENMP ON)
endif ()

if (SVTK_SMP_IMPLEMENTATION_TYPE STREQUAL "TBB")
  set(SVTKm_ENABLE_TBB ON)
endif ()

set(SVTKm_ENABLE_CUDA ${SVTK_USE_CUDA})

set(SVTKm_INSTALL_LIB_DIR "${_svtk_build_LIBRARY_DESTINATION}")
set(SVTKm_INSTALL_INCLUDE_DIR "${_svtk_build_HEADERS_DESTINATION}/svtksvtkm/svtk-m")
if (DEFINED _svtk_build_LIBRARY_NAME_SUFFIX)
  set(SVTKm_CUSTOM_LIBRARY_SUFFIX "-${_svtk_build_LIBRARY_NAME_SUFFIX}")
endif ()
set(SVTKm_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(SVTKm_LIBRARY_OUTPUT_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(SVTKm_BUILD_CMAKE_BASE_DIR "${CMAKE_BINARY_DIR}")
set(SVTKm_INSTALL_CONFIG_DIR "${_svtk_build_CMAKE_DESTINATION}/svtkm")
set(SVTKm_INSTALL_CMAKE_MODULE_DIR "${SVTKm_INSTALL_CONFIG_DIR}/cmake")

if (_svtk_build_INSTALL_HEADERS)
  set(SVTKm_INSTALL_ONLY_LIBRARIES OFF)
else ()
  set(SVTKm_INSTALL_ONLY_LIBRARIES ON)
endif ()

set(SVTKm_USE_DEFAULT_SYMBOL_VISIBILITY OFF)
set(SVTKm_ENABLE_RENDERING OFF)
set(SVTKm_ENABLE_TESTING OFF)
set(SVTKm_ENABLE_BENCHMARKS OFF)
set(SVTKm_ENABLE_DOCUMENTATION OFF)
set(SVTKm_ENABLE_EXAMPLES OFF)
set(SVTKm_ENABLE_CPACK OFF)
set(SVTKm_USE_DOUBLE_PRECISION OFF)
set(SVTKm_USE_64BIT_IDS "${SVTK_USE_64BIT_IDS}")
set(SVTKm_NO_ASSERT ON)

# SVTK-m uses stricter warning checks resulting in more warnings when SVTK-m
# is enabled.
# Disable SVTK-m warning flags and just rely on SVTK's warning flags.
set(SVTKm_ENABLE_DEVELOPER_FLAGS OFF)

# Workaround cmake issue #7519 which causes the c++11 flag set for SVTK-m using
# target_compile_features to not work.
if (SVTKm_ENABLE_CUDA AND CMAKE_VERSION VERSION_LESS 3.11)
  set(CMAKE_CUDA_STANDARD 11)
  set(CMAKE_CUDA_STANDARD_REQUIRED True)
endif ()

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(svtk-m)
svtkm_setup_job_pool()

svtk_module_add_module(SVTK::svtkm
  HEADER_ONLY)
svtk_module_link(SVTK::svtkm INTERFACE svtkm_cont svtkm_filter)
