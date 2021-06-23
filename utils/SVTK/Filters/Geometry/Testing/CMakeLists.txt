svtk_module_test_data(
  Data/explicitStructuredGrid.vtu
  Data/ghost_cells.vtu
  Data/quadraticTetra01.vtu
  Data/quadratic_tets_with_ghost_cells_0.vtu)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/RectGrid2.svtk
    Data/combq.bin
    Data/combxyz.bin
    Data/office.binary.svtk)

  add_subdirectory(Python)
endif ()
