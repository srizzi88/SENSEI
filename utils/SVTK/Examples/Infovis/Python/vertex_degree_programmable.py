#!/usr/bin/env python
from svtk import *

vertexDegree = svtkProgrammableFilter()

def computeVertexDegree():
  input = vertexDegree.GetInput()
  output = vertexDegree.GetOutput()

  output.ShallowCopy(input)

  # Create output array
  vertexArray = svtkIntArray()
  vertexArray.SetName("VertexDegree")
  vertexArray.SetNumberOfTuples(output.GetNumberOfVertices())

  # Loop through all the vertices setting the degree for the new attribute array
  for i in range(output.GetNumberOfVertices()):
      vertexArray.SetValue(i, output.GetDegree(i))

  # Add the new attribute array to the output graph
  output.GetVertexData().AddArray(vertexArray)

vertexDegree.SetExecuteMethod(computeVertexDegree)


# SVTK Pipeline
randomGraph = svtkRandomGraphSource()
randomGraph.SetNumberOfVertices(25)
randomGraph.SetStartWithTree(True)
vertexDegree.AddInputConnection(randomGraph.GetOutputPort())

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(vertexDegree.GetOutputPort())
view.SetVertexColorArrayName("VertexDegree")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
view.SetVertexLabelFontSize(20)

theme = svtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(3)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()
view.ResetCamera()
view.Render()
view.GetInteractor().Start()

