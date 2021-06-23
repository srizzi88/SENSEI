add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/headsq/,REGEX:.*
    Data/track1.binary.svtk
    Data/track2.binary.svtk
    Data/track3.binary.svtk)

  add_subdirectory(Python)
endif ()
