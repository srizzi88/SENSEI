svtk_add_test_cxx(svtkRenderingOpenVRCxxTests tests
  TestAvatar.cxx
#  Medical.cxx,NO_VALID
  )

if (WIN32)
  svtk_add_test_cxx(svtkRenderingOpenVRCxxTests tests
    TestDragon.cxx,NO_VALID
    )
endif ()

svtk_test_cxx_executable(svtkRenderingOpenVRCxxTests tests RENDERING_FACTORY)
