set(classes
  svtkSMPContourGrid
  svtkSMPMergePoints
  svtkSMPMergePolyDataHelper)

if (NOT SVTK_LEGACY_REMOVE)
  list(APPEND classes
    svtkSMPContourGridManyPieces
    svtkThreadedSynchronizedTemplates3D
    svtkThreadedSynchronizedTemplatesCutter3D
    svtkSMPTransform
    svtkSMPWarpVector)
endif()

svtk_module_add_module(SVTK::FiltersSMP
  CLASSES ${classes})
