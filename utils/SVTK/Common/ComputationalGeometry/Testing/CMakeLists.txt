svtk_module_test_data(
  Data/beach.jpg)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
