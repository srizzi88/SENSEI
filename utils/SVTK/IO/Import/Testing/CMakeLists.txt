svtk_module_test_data(
  Data/WineGlass.wrl)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/bot2.wrl)

  add_subdirectory(Python)
endif ()
