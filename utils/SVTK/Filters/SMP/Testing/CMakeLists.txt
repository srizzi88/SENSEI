add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/disk_out_ref.ex2)

  add_subdirectory(Python)
endif ()
