svtk_module_test_data(
  Data/2LYZ.pdb
  Data/Microscopy/small2.ndpi)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/RectGrid2.svtk)

  add_subdirectory(Python)
endif ()
