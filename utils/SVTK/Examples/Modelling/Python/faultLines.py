#!/usr/bin/env python

# Create a constrained Delaunay triangulation following fault lines. The
# fault lines serve as constraint edges in the Delaunay triangulation.

import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.colors import *
SVTK_DATA_ROOT = svtkGetDataRoot()

# Generate some points by reading a SVTK data file. The data file also
# has edges that represent constraint lines. This is originally from a
# geologic horizon.
reader = svtk.svtkPolyDataReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/faults.svtk")

# Perform a 2D triangulation with constraint edges.
delny = svtk.svtkDelaunay2D()
delny.SetInputConnection(reader.GetOutputPort())
delny.SetSourceConnection(reader.GetOutputPort())
delny.SetTolerance(0.00001)
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(delny.GetOutputPort())
mapMesh = svtk.svtkPolyDataMapper()
mapMesh.SetInputConnection(normals.GetOutputPort())
meshActor = svtk.svtkActor()
meshActor.SetMapper(mapMesh)
meshActor.GetProperty().SetColor(beige)

# Now pretty up the mesh with tubed edges and balls at the vertices.
tuber = svtk.svtkTubeFilter()
tuber.SetInputConnection(reader.GetOutputPort())
tuber.SetRadius(25)
mapLines = svtk.svtkPolyDataMapper()
mapLines.SetInputConnection(tuber.GetOutputPort())
linesActor = svtk.svtkActor()
linesActor.SetMapper(mapLines)
linesActor.GetProperty().SetColor(1, 0, 0)
linesActor.GetProperty().SetColor(tomato)

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(linesActor)
ren.AddActor(meshActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(350, 250)

cam1 = svtk.svtkCamera()
cam1.SetClippingRange(2580, 129041)
cam1.SetFocalPoint(461550, 6.58e+006, 2132)
cam1.SetPosition(463960, 6.559e+06, 16982)
cam1.SetViewUp(-0.321899, 0.522244, 0.78971)
light = svtk.svtkLight()
light.SetPosition(0, 0, 1)
light.SetFocalPoint(0, 0, 0)
ren.SetActiveCamera(cam1)
ren.AddLight(light)

ren.GetActiveCamera().Zoom(1.5)
iren.LightFollowCameraOff()

iren.Initialize()
renWin.Render()
iren.Start()
