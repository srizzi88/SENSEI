svtk_add_test_cxx(svtkRenderingFreeTypeFontConfigCxxTests tests
  TestSystemFontRendering.cxx
  )
svtk_test_cxx_executable(svtkRenderingFreeTypeFontConfigCxxTests tests
    RENDERING_FACTORY)
