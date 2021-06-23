set(classes
  svtkDIYUtilities
  svtkDIYExplicitAssigner)

svtk_module_add_module(SVTK::ParallelDIY
  CLASSES ${classes})

svtk_module_definitions(SVTK::ParallelDIY
  PUBLIC DIY_NO_THREADS)
