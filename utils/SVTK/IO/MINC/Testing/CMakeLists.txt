if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/masonry.bmp
    Data/mni-surface-mesh.obj
    Data/t3_grid_0.mnc)

  add_subdirectory(Python)
endif ()
