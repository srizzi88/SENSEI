if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/bluntfinq.bin
    Data/bluntfinxyz.bin
    Data/earth.ppm
    Data/masonry.bmp
    Data/texThres2.svtk)

  add_subdirectory(Python)
endif ()
