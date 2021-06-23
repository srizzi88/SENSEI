set(classes
  svtkAnnotatedCubeActor
  svtkArcPlotter
  svtkAxesActor
  svtkAxisActor
  svtkAxisActor2D
  svtkAxisFollower
  svtkBarChartActor
  svtkCaptionActor2D
  svtkConvexHull2D
  svtkCornerAnnotation
  svtkCubeAxesActor
  svtkCubeAxesActor2D
  svtkGraphAnnotationLayersFilter
  svtkLeaderActor2D
  svtkLegendBoxActor
  svtkLegendScaleActor
  svtkParallelCoordinatesActor
  svtkPieChartActor
  svtkPolarAxesActor
  svtkProp3DAxisFollower
  svtkScalarBarActor
  svtkSpiderPlotActor
  svtkXYPlotActor)

set(headers
  svtkScalarBarActorInternal.h)

svtk_module_add_module(SVTK::RenderingAnnotation
  CLASSES ${classes}
  HEADERS ${headers})
