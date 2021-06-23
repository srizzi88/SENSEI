svtk_module_test_data(
  Data/SampleStructGrid.svtk
  Data/cow.vtp)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
