set(enable_large_data_tests FALSE)
if (SVTK_USE_LARGE_DATA AND NOT WIN32)
  if (DEFINED SVTK_WORDS_BIGENDIAN AND NOT SVTK_WORDS_BIGENDIAN)
    svtk_module_test_data(
      Data/WindBladeReader/,REGEX:.*
      Data/WindBladeReader/field/,REGEX:.*
      Data/WindBladeReader/turbine/,REGEX:.*)
    set(enable_large_data_tests TRUE)
  endif ()
endif ()

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/multi-bin.xyz
    Data/multi-bin-oflow.q
    Data/multi-bin.f)

  add_subdirectory(Python)
endif ()
