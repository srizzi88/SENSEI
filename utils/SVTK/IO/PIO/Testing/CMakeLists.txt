svtk_module_test_data(
  Data/PIO/simple.pio
  Data/PIO/dumps.simple/simple-dmp000000
  Data/PIO/dumps.simple/simple-dmp000010
)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
