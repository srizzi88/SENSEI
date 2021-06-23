add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/combq.bin
    Data/combxyz.bin)

  add_subdirectory(Python)
endif ()
