#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.SetIncludeEdgeWeights(True)

degree = svtkVertexDegree()
degree.AddInputConnection(source.GetOutputPort())

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(degree.GetOutputPort())
view.SetVertexLabelArrayName("VertexDegree")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
view.SetEdgeLabelArrayName("edge weight")
view.SetEdgeLabelVisibility(True)
view.SetEdgeColorArrayName("edge weight")
view.SetColorEdges(True)
view.SetLayoutStrategyToSimple2D()

theme = svtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(4)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()

