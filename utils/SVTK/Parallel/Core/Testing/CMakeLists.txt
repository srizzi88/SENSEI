svtk_module_test_data(
  Data/RectGrid2.svtk
  Data/blow.svtk
  Data/combq.bin
  Data/combxyz.bin
  Data/earth.ppm
  Data/teapot.g
  )

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
