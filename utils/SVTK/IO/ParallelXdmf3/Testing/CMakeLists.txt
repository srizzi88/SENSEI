if (SVTK_USE_LARGE_DATA)
  svtk_module_test_data(
    Data/XDMF/Iron/,REGEX:Iron_Protein\.ImageData\..*)

  add_subdirectory(Cxx)
endif ()
