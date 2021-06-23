svtk_module_test_data(
  Data/GreenCircle.png
  Data/SainteHelens.dem
  Data/SampleStructGrid.svtk
  Data/Tango/TangoIcons.png
  Data/alphachannel.png
  Data/beach.jpg
  Data/can.ex2
  Data/chombo3d/,REGEX:.*
  Data/combq.bin
  Data/combxyz.bin
  Data/earth.ppm
  Data/quadraticTetra01.vtu
  Data/textureRGBA.png)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/Viewpoint/cow.g
    Data/Viewpoint/cow.obj
    Data/beach.tif
    Data/blow.svtk
    Data/headsq/,REGEX:.*
    Data/masonry.bmp)

  add_subdirectory(Python)
endif ()
