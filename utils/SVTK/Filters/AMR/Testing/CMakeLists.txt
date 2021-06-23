add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/AMR/Enzo/DD0010/,REGEX:.*)

  add_subdirectory(Python)
endif ()
