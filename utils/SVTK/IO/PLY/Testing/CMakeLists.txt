svtk_module_test_data(
  Data/Armadillo.ply
  Data/PointCloud.ply
  Data/bunny.ply
  Data/squareTextured.ply
  Data/squareTexturedFaces.ply
  Data/svtk.png
  Data/two_svtk_logos_stacked.png)

add_subdirectory(Cxx)

if (SVTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
