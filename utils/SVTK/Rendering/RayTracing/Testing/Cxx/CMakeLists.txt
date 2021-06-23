if (SVTK_ENABLE_OSPRAY)
  svtk_add_test_cxx(svtkRenderingRayTracingCxxTests tests
    # Surface rendering tests
    TestOSPRayAmbient.cxx
    TestOSPRayAMRVolumeRenderer.cxx
    TestOSPRayCompositePolyDataMapper2.cxx
    TestOSPRayDepthOfField.cxx
    TestOSPRayDynamicObject.cxx
    TestOSPRayDynamicScene.cxx
    TestOSPRayImplicits.cxx
    TestOSPRayIsosurface.cxx
    TestOSPRayLayers.cxx
    TestOSPRayLights.cxx
    TestOSPRayMultiBlock.cxx
    TestOSPRayMultiBlockPartialArrayFieldData.cxx
    TestOSPRayOrthographic.cxx
    TestOSPRayPass.cxx
    TestOSPRayRenderMesh.cxx
    TestOSPRayRotatedNormals.cxx
    TestOSPRayScalarBar.cxx
    TestOSPRayTetrahedraMapper.cxx
    TestOSPRayTetrahedraMapperMixedCells.cxx
    TestOSPRayTime.cxx
    TestOSPRayWindow.cxx
    TestOSPRayStereo.cxx
    TestRayTracedMolecules.cxx
    # Volume rendering tests
    TestGPURayCastCameraInside.cxx
    TestGPURayCastCellData.cxx
    TestGPURayCastCameraInsideSmallSpacing.cxx
    TestGPURayCastMapperBenchmark.cxx
    TestGPURayCastMapperSampleDistance.cxx
    TestGPURayCastPerspectiveParallel.cxx
    #TestGPURayCastPositionalLights.cxx
    TestGPURayCastVolumeLightKit.cxx
    TestGPURayCastVolumePolyData.cxx
    TestGPURayCastVolumeScale.cxx
    TestGPURayCastVolumeUpdate.cxx
    TestGPUVolumeRayCastMapper.cxx
    TestOSPRayVolumeRenderer.cxx
    TestOSPRayVolumeRendererCrop.cxx
    TestSmartVolumeMapper.cxx
    # Path tracer tests
    TestCategoricalMaterials.cxx
    TestCategoricalMultiBlock.cxx
    TestPathTracerBackground.cxx
    TestPathTracerLuminous.cxx
    TestPathTracerMaterialLibrary.cxx,NO_VALID
    TestPathTracerMaterials.cxx
    TestPathTracerShadows.cxx
    # Miscellaneous tests
    TestOSPRayCache.cxx
    TestOSPRayTiling.cxx
  )
endif()

if (SVTK_ENABLE_VISRTX)
  # OSPRay tests that are rerun with the OptiX backend
  set(optix_tests
    TestOSPRayAmbient
    TestOSPRayCompositePolyDataMapper2
    TestOSPRayDepthOfField
    TestOSPRayDynamicObject
    TestOSPRayDynamicScene
    TestOSPRayImplicits
    TestOSPRayLayers
    TestOSPRayLights
    TestOSPRayMultiBlock
    TestOSPRayMultiBlockPartialArrayFieldData
    TestOSPRayOrthographic
    TestOSPRayPass
    TestOSPRayRenderMesh
    TestOSPRayRotatedNormals
    TestOSPRayScalarBar
    TestOSPRayTime
    TestOSPRayWindow
    TestOSPRayStereo
    TestCategoricalMaterials
    TestCategoricalMultiBlock
    TestPathTracerBackground
    TestPathTracerLuminous
    TestPathTracerMaterials
    TestPathTracerShadows
    TestOSPRayCache
    TestOSPRayTiling
    )

  set(testname_suffix "OptiX")
  set(optix_test_sources)
  foreach (tname IN LISTS optix_tests)
    list(APPEND optix_test_sources ${tname}${testname_suffix},${tname}.cxx)
  endforeach ()

  svtk_add_test_cxx(svtkRenderingRayTracingCxxTests tests_optix
    "--OptiX"
    ${optix_test_sources})
endif ()

if (SVTK_ENABLE_OSPRAY AND SVTK_ENABLE_VISRTX)
  # Tests combining both OSPRay and OptiX backends
  svtk_add_test_cxx(svtkRenderingRayTracingCxxTests tests_combined
    TestRendererType.cxx
    TestMultipleBackends.cxx
    )
endif ()

set(all_tests ${tests} ${tests_optix} ${tests_combined})
svtk_test_cxx_executable(svtkRenderingRayTracingCxxTests all_tests RENDERING_FACTORY
  svtkOSPRayTestInteractor.cxx
  )


# Do we have MPI and parallel support
if (SVTK_ENABLE_OSPRAY AND TARGET svtkRenderingParallel AND TARGET svtkParallelMPI)
  set(svtkRenderingRayTracingCxx-MPI_NUMPROCS 2)
  svtk_add_test_mpi(svtkRenderingRayTracingCxx-MPI tests
    TESTING_DATA
    TestOSPRayComposite.cxx
    )
  svtk_test_cxx_executable(svtkRenderingRayTracingCxx-MPI tests RENDERING_FACTORY
    svtkOSPRayTestInteractor.cxx
    )

  target_link_libraries(svtkRenderingRayTracingCxx-MPI
    PRIVATE
      svtkRenderingParallel
      svtkParallelMPI)
endif ()
