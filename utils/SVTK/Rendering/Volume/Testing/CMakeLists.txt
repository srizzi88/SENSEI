svtk_module_test_data(
  Data/HeadMRVolume.mhd
  Data/MagnitudeImage_256x256x148
  Data/headsq/,REGEX:.*
  Data/headmr3blocks/headmr3blocks.vtm
  Data/headmr3blocks/headmr3blocks/,REGEX:.*
  Data/hncma-atlas.vti
  Data/ironProt.svtk
  Data/neghip.slc
  Data/sphere.slc
  Data/tooth.nhdr
  Data/tooth.raw
  Data/tooth_2dtransf.png
  Data/vase_1comp.vti
  Data/vase_4comp.vti)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/m4_TotalDensity.cube)

  add_subdirectory(Python)
endif ()
