#!/usr/bin/env python
from svtk import *

csv_source = svtkDelimitedTextReader()
csv_source.SetFieldDelimiterCharacters(",")
csv_source.SetHaveHeaders(True)
csv_source.SetDetectNumericColumns(True)
csv_source.SetFileName("authors.csv")

graph = svtkTableToGraph()
graph.AddInputConnection(csv_source.GetOutputPort())
graph.AddLinkVertex("Author", "Name", False)
graph.AddLinkVertex("Employer", "Company", False)
graph.AddLinkEdge("Author", "Employer")

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())
view.SetVertexLabelArrayName("label")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("ids")
view.SetColorVertices(True)
view.SetLayoutStrategyToFast2D()

theme = svtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(5)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
view.SetVertexLabelFontSize(20)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()

