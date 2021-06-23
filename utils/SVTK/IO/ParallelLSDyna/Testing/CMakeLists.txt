if (SVTK_USE_LARGE_DATA)
  svtk_module_test_data(
    Data/LSDyna/hemi.draw/,REGEX:.*)

  add_subdirectory(Cxx)
endif ()
