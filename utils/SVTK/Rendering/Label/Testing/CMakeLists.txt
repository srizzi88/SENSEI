svtk_module_test_data(
  Data/uniform-001371-5x5x5.vtp)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/headsq/,REGEX:.*)

  add_subdirectory(Python)
endif ()
