set(classes
  svtkOggTheoraWriter)

svtk_module_add_module(SVTK::IOOggTheora
  CLASSES ${classes})

# this is for legacy codes, should be dropped.
svtk_module_definitions(SVTK::IOOggTheora
  INTERFACE SVTK_HAS_OGGTHEORA_SUPPORT)
