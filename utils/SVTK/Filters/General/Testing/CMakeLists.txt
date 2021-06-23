svtk_module_test_data(
  Data/Quadratic/CylinderQuadratic.svtk
  Data/Tango/TangoIcons.png
  Data/UCD2D/UCD_00005.inp
  Data/fullhead15.png
  Data/hexa.svtk
  Data/mine_with_dates.vtp
  Data/quadraticTetra01.vtu
  Data/waveletMaterial.vti)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/42400-IDGH.stl
    Data/EnSight/,REGEX:elements\..*
    Data/RectGrid2.svtk
    Data/RedCircle.png
    Data/SampleStructGrid.svtk
    Data/camscene.png
    Data/can.ex2
    Data/combq.bin
    Data/combxyz.bin
    Data/earth.ppm
    Data/headsq/,REGEX:.*
    Data/ironProt.svtk
    Data/office.binary.svtk
    Data/uGridEx.svtk)

  add_subdirectory(Python)
endif ()
