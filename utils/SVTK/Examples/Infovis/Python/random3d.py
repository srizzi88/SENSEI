#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.SetNumberOfVertices(150)
source.SetEdgeProbability(0.01)
source.SetUseEdgeProbability(True)
source.SetStartWithTree(True)

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())
view.SetVertexLabelArrayName("vertex id")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("vertex id")
view.SetColorVertices(True)
view.SetLayoutStrategyToSpanTree()
view.SetInteractionModeTo3D() # Left mouse button causes 3D rotate instead of zoom
view.SetLabelPlacementModeToNoOverlap()

theme = svtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()
