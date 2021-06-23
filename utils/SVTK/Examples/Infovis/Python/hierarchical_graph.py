#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.SetNumberOfVertices(200)
source.SetEdgeProbability(0.01)
source.SetUseEdgeProbability(True)
source.SetStartWithTree(True)
source.IncludeEdgeWeightsOn()
source.AllowParallelEdgesOn()

# Connect to the svtkVertexDegree filter.
degree_filter = svtkVertexDegree()
degree_filter.SetOutputArrayName("vertex_degree")
degree_filter.SetInputConnection(source.GetOutputPort())


# Connect to the boost breath first search filter.
mstTreeSelection = svtkBoostKruskalMinimumSpanningTree()
mstTreeSelection.SetInputConnection(degree_filter.GetOutputPort())
mstTreeSelection.SetEdgeWeightArrayName("edge weight")
mstTreeSelection.NegateEdgeWeightsOn()


# Take selection and extract a graph
extract_graph = svtkExtractSelectedGraph()
extract_graph.AddInputConnection(degree_filter.GetOutputPort())
extract_graph.SetSelectionConnection(mstTreeSelection.GetOutputPort())

# Create a tree from the graph :)
bfsTree = svtkBoostBreadthFirstSearchTree()
bfsTree.AddInputConnection(extract_graph.GetOutputPort())

treeStrat = svtkTreeLayoutStrategy();
treeStrat.RadialOn()
treeStrat.SetAngle(360)
treeStrat.SetLogSpacingValue(1)

forceStrat = svtkSimple2DLayoutStrategy()
forceStrat.SetEdgeWeightField("edge weight")

dummy = svtkHierarchicalGraphView()

# Create Tree/Graph Layout view
view = svtkHierarchicalGraphView()
view.SetHierarchyFromInputConnection(bfsTree.GetOutputPort())
view.SetGraphFromInputConnection(degree_filter.GetOutputPort())
view.SetVertexColorArrayName("VertexDegree")
view.SetColorVertices(True)
view.SetVertexLabelArrayName("VertexDegree")
view.SetVertexLabelVisibility(True)
view.SetEdgeColorArrayName("edge weight")
# FIXME: If you uncomment this line the display locks up
view.SetColorEdges(True)
view.SetEdgeLabelArrayName("edge weight")
view.SetEdgeLabelVisibility(True)
view.SetLayoutStrategy(forceStrat)
view.SetBundlingStrength(.8)


# Set up the theme
theme = svtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()
