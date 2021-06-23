svtk_module_test_data(
  Data/isofill_0.vtp)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
