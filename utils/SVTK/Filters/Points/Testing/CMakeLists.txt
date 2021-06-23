add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/SPH_Points.vtu
    Data/SPH_Points2D.vtu
    Data/SainteHelens.dem
    Data/combq.bin
    Data/combxyz.bin)

  add_subdirectory(Python)
endif ()
