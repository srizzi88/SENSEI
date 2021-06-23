set(classes
  svtkBufferedArchiver
  svtkPartitionedArchiver)

svtk_module_find_package(PACKAGE LibArchive)

svtk_module_add_module(SVTK::CommonArchive
  CLASSES ${classes})

svtk_module_link(SVTK::CommonArchive PRIVATE LibArchive::LibArchive)
