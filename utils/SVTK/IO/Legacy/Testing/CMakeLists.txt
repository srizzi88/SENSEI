svtk_module_test_data(
  Data/ghost_cells.svtk)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
