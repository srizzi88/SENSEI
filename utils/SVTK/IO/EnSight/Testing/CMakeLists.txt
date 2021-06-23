if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/EnSight/,REGEX:.*)

  add_subdirectory(Python)
endif ()
