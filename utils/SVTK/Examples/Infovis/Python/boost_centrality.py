#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.DirectedOff()
source.SetNumberOfVertices(50)
source.SetEdgeProbability(0.01)
source.SetUseEdgeProbability(True)
source.AllowParallelEdgesOn()
source.AllowSelfLoopsOn()
source.SetStartWithTree(True)


# Connect to the Boost centrality filter.
centrality = svtkBoostBrandesCentrality ()
centrality.SetInputConnection(source.GetOutputPort())


view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(centrality.GetOutputPort())
view.SetVertexLabelArrayName("centrality")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("centrality")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("centrality")
#view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("centrality")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

theme = svtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetPointSize(10)
theme.SetCellOpacity(1)
theme.SetVertexLabelColor(0, 0, 0)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()

