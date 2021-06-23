set(classes
  svtkAbstractContextBufferId
  svtkAbstractContextItem
  svtkBlockItem
  svtkBrush
  svtkContext2D
  svtkContext3D
  svtkContextActor
  svtkContextClip
  svtkContextDevice2D
  svtkContextDevice3D
  svtkContextItem
  svtkContextKeyEvent
  svtkContextMapper2D
  svtkContextMouseEvent
  svtkContextScene
  svtkContextTransform
  svtkImageItem
  svtkLabeledContourPolyDataItem
  svtkMarkerUtilities
  svtkPen
  svtkPolyDataItem
  svtkPropItem
  svtkTooltipItem)

svtk_module_add_module(SVTK::RenderingContext2D
  CLASSES ${classes})
