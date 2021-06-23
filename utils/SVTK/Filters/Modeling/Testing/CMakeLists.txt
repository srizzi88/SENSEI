svtk_module_test_data(
  Data/SemiDisk/SemiDisk-0.vtp
  Data/SemiDisk/SemiDisk-1.vtp)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/SainteHelens.dem
    Data/headsq/,REGEX:.*
    Data/ironProt.svtk
    Data/nut.slc
    Data/ruler.png
    Data/usa.svtk
    Data/svtk.svtk)

  add_subdirectory(Python)
endif ()
