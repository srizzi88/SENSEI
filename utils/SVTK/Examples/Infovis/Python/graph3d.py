#!/usr/bin/env python
from svtk import *
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtkXGMLReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/Infovis/fsm.gml")
reader.Update()

strategy   = svtkSpanTreeLayoutStrategy()
strategy.DepthFirstSpanningTreeOn()

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(reader.GetOutputPort())
view.SetVertexLabelArrayName("vertex id")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("vertex id")
view.SetColorVertices(True)
view.SetLayoutStrategy( strategy )
view.SetInteractionModeTo3D() # Left mouse button causes 3D rotate instead of zoom

theme = svtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(2)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

#Here's the window with David's original layout methodology
#  Aside from the theme elements in the view above, the notable
#  difference between the two views is the angling on the edges.
layout = svtkGraphLayout()
layout.SetLayoutStrategy(strategy)
layout.SetInputConnection(reader.GetOutputPort())

edge_geom = svtkGraphToPolyData()
edge_geom.SetInputConnection(layout.GetOutputPort())

vertex_geom = svtkGraphToPoints()
vertex_geom.SetInputConnection(layout.GetOutputPort())

# Vertex pipeline - mark each vertex with a cube glyph
cube = svtkCubeSource()
cube.SetXLength(0.3)
cube.SetYLength(0.3)
cube.SetZLength(0.3)

glyph = svtkGlyph3D()
glyph.SetInputConnection(vertex_geom.GetOutputPort())
glyph.SetSourceConnection(0, cube.GetOutputPort())

gmap = svtkPolyDataMapper()
gmap.SetInputConnection(glyph.GetOutputPort())

gact = svtkActor()
gact.SetMapper(gmap)
gact.GetProperty().SetColor(0,0,1)

# Edge pipeline - map edges to lines
mapper = svtkPolyDataMapper()
mapper.SetInputConnection(edge_geom.GetOutputPort())

actor = svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(0.4,0.4,0.6)

# Renderer, window, and interaction
ren = svtkRenderer()
ren.AddActor(actor)
ren.AddActor(gact)
ren.ResetCamera()

renWin = svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(800,550)

iren = svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
#iren.Start()

view.GetInteractor().Start()
