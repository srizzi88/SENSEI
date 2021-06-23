set(classes
  svtkDynamic2DLabelMapper
  svtkFreeTypeLabelRenderStrategy
  svtkLabeledDataMapper
  svtkLabeledTreeMapDataMapper
  svtkLabelHierarchyAlgorithm
  svtkLabelHierarchyCompositeIterator
  svtkLabelHierarchy
  svtkLabelHierarchyIterator
  svtkLabelPlacementMapper
  svtkLabelPlacer
  svtkLabelRenderStrategy
  svtkLabelSizeCalculator
  svtkPointSetToLabelHierarchy)

svtk_module_add_module(SVTK::RenderingLabel
  CLASSES ${classes})
