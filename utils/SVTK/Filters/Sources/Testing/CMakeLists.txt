svtk_module_test_data(
  Data/mbwavelet_ascii.q
  Data/mbwavelet_ascii.xyz)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/beach.jpg
    Data/earth.ppm)

  add_subdirectory(Python)
endif ()
