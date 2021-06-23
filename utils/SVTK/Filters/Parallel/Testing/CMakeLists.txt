svtk_module_test_data(
  Data/RectGrid2.svtk
  Data/SampleStructGrid.svtk
  Data/ironProt.svtk
  Data/periodicPiece.vtu
  Data/tetraMesh.svtk
  Data/two_svtk_logos_stacked.png)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  svtk_module_test_data(
    Data/RectGrid2.vtr
    Data/cth.vtr
    Data/elements.vtu
    Data/headsq/,REGEX:.*
    Data/,REGEX:simpleamrgrid\\.e-s[0-9]+)

  add_subdirectory(Python)
endif ()
