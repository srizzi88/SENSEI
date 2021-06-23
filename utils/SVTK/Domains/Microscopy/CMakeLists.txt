set(classes
  svtkOpenSlideReader)

svtk_module_add_module(SVTK::DomainsMicroscopy
  CLASSES ${classes})

svtk_module_find_package(PACKAGE OpenSlide)
svtk_module_link(SVTK::DomainsMicroscopy
  PUBLIC
    OpenSlide::OpenSlide)
