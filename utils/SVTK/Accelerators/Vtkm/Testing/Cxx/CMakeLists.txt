
svtk_add_test_cxx(svtkAcceleratorsSVTKmCxxTests1 tests1
  TestSVTKMCleanGrid.cxx
  TestSVTKMCoordinateSystemTransform.cxx,NO_VALID
  TestSVTKMClip.cxx
  TestSVTKMClipWithImplicitFunction.cxx
  TestSVTKMExternalFaces.cxx
  TestSVTKMExtractVOI.cxx
  TestSVTKMGradient.cxx,NO_VALID
  TestSVTKMGradientAndVorticity.cxx,NO_VALID
  TestSVTKMHistogram.cxx,NO_VALID
  TestSVTKMLevelOfDetail.cxx
  TestSVTKMMarchingCubes.cxx
  TestSVTKMMarchingCubes2.cxx
  TestSVTKMNDHistogram.cxx,NO_VALID
  TestSVTKMPointElevation.cxx
  TestSVTKMPointTransform.cxx
  TestSVTKMProbe.cxx,NO_VALID
  TestSVTKMPolyDataNormals.cxx
  TestSVTKMThreshold.cxx
  TestSVTKMThreshold2.cxx
  TestSVTKMTriangleMeshPointNormals.cxx
  TestSVTKMWarpScalar.cxx
  TestSVTKMWarpVector.cxx
  )

# We are splitting the tests into two executables to workaround an issue in
# cuda. With all the tests in the same executable several tests are failing
# in cuda. We have not identified the root cause of the problem yet.
svtk_add_test_cxx(svtkAcceleratorsSVTKmCxxTests2 tests2
  TestSVTKMDataArray.cxx,NO_VALID
  TestSVTKMDataSet.cxx,NO_VALID
  )

if (TARGET svtkm::cuda)
  #the enable_language call is scoped! so we have to re-enable
  #cuda in the test directory
  enable_language(CUDA)
  foreach(src IN LISTS tests1)
    set_source_files_properties(${src}.cxx PROPERTIES LANGUAGE "CUDA")
  endforeach()
  foreach(src IN LISTS tests2)
    set_source_files_properties(${src}.cxx PROPERTIES LANGUAGE "CUDA")
  endforeach()

  #the tests aren't scoped as a child directory of svtkAcceleratorsSVTKm
  #so we need to redo this logic
  svtkm_get_cuda_flags(CMAKE_CUDA_FLAGS)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif()


svtk_test_cxx_executable(svtkAcceleratorsSVTKmCxxTests1 tests1
  RENDERING_FACTORY
  )

svtk_test_cxx_executable(svtkAcceleratorsSVTKmCxxTests2 tests2
  RENDERING_FACTORY
  )

if (TARGET svtkm::cuda)
  # When cuda is enabled SVTK::AcceleratorsSVTKm is built statically but with fpic
  # enabled so the tests are also built with fpic enabled
  set_target_properties(svtkAcceleratorsSVTKmCxxTests1 PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(svtkAcceleratorsSVTKmCxxTests2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
