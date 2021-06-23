svtk_add_test_cxx(svtkFiltersSMPCxxTests tests
  NO_VALID
  TestSMPContour.cxx
  )
if (NOT SVTK_LEGACY_REMOVE)
    svtk_add_test_cxx(svtkFiltersSMPCxxTests tests
      NO_VALID
      TestThreadedSynchronizedTemplates3D.cxx
      TestThreadedSynchronizedTemplatesCutter3D.cxx
      TestSMPTransform.cxx
      TestSMPWarp.cxx
      )
endif()
svtk_test_cxx_executable(svtkFiltersSMPCxxTests tests)
