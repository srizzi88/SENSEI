if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/SainteHelens.dem
    Data/beach.tif)

  add_subdirectory(Python)
endif ()
