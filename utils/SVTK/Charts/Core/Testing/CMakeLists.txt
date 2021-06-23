svtk_module_test_data(
  Data/SainteHelens.dem
  Data/svtk.png
  Data/vehicle_data.csv)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
