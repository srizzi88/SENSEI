svtk_add_test_cxx(svtkIOFFMPEGCxxTests tests
  TestFFMPEGWriter.cxx,NO_VALID
  )

if (SVTK_FFMPEG_AVCODEC_SEND_PACKET)
  svtk_add_test_cxx(svtkIOFFMPEGCxxTests tests
    TestFFMPEGVideoSource.cxx
    )
if (WIN32)
  svtk_add_test_cxx(svtkIOFFMPEGCxxTests tests
    TestFFMPEGVideoSourceWithAudio.cxx
    )
  set(extra_test_libs Xaudio2.lib)
endif()
endif()

svtk_test_cxx_executable(svtkIOFFMPEGCxxTests tests)
if (extra_test_libs)
set_property(TARGET svtkIOFFMPEGCxxTests APPEND PROPERTY LINK_LIBRARIES ${extra_test_libs})
endif()
