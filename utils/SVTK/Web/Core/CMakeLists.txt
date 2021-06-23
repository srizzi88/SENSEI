set(classes
  svtkDataEncoder
  svtkObjectIdMap
  svtkWebApplication
  svtkWebInteractionEvent
  svtkWebUtilities)

svtk_module_add_module(SVTK::WebCore
  CLASSES ${classes})
