svtk_module_test_data(
  Data/cow.vtp)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/SainteHelens.dem
    Data/clown.facet
    Data/earth.ppm
    Data/headsq/,REGEX:.*
    Data/iflamigm.3ds
    Data/masonry.bmp
    Data/svtk.png)

  add_subdirectory(Python)
endif ()
