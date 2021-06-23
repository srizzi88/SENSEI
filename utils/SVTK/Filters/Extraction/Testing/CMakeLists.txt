svtk_module_test_data(
  Data/cad_cubes.vtp
  Data/can.ex2)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/RectGrid2.svtk
    Data/blow.svtk
    Data/combq.bin
    Data/combxyz.bin)

  add_subdirectory(Python)
endif ()
