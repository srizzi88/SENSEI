#!/usr/bin/env python
from svtk import *

graph = svtkMutableDirectedGraph()
a = graph.AddVertex()
b = graph.AddChild(a)
c = graph.AddChild(a)
d = graph.AddChild(b)
e = graph.AddChild(c)
f = graph.AddChild(c)

tree = svtkTree()
tree.CheckedShallowCopy(graph)

view = svtkGraphLayoutView()
view.AddRepresentationFromInput(tree)

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()


