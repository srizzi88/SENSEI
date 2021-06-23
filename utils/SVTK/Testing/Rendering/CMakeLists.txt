set(classes
  svtkTesting
  svtkTestingInteractor
  svtkTestingObjectFactory)

set(headers
  svtkMultiBaselineRegressionTest.h
  svtkRegressionTestImage.h)

svtk_module_add_module(SVTK::TestingRendering
  CLASSES ${classes}
  HEADERS ${headers})
