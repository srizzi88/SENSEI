project(mpi4py)

add_subdirectory(src)

svtk_module_add_module(SVTK::mpi4py
  HEADER_ONLY)
target_include_directories(mpi4py
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/mpi4py/include>"
    "$<INSTALL_INTERFACE:${_svtk_build_HEADERS_DESTINATION}/svtkmpi4py>")
