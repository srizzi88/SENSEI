#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.SetNumberOfVertices(15)
source.SetStartWithTree(True)
source.SetIncludeEdgeWeights(True)

bfs = svtkBoostBreadthFirstSearch()
bfs.AddInputConnection(source.GetOutputPort())
bfs.SetOriginVertex(0)

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(bfs.GetOutputPort())
view.SetVertexLabelArrayName("BFS")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("BFS")
view.SetColorVertices(True)
view.SetEdgeColorArrayName("edge weight")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()
view.SetVertexLabelFontSize(20)

theme = svtkViewTheme.CreateNeonTheme()
theme.SetLineWidth(5)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()


view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()

