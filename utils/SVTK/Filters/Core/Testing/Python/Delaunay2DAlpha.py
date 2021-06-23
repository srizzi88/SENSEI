#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create some points
#
math = svtk.svtkMath()
points = svtk.svtkPoints()
i = 0
while i < 100:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = svtk.svtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = svtk.svtkDelaunay2D()
del1.SetInputData(profile)
del1.SetTolerance(0.001)
del1.SetAlpha(0.1)
shrink = svtk.svtkShrinkPolyData()
shrink.SetInputConnection(del1.GetOutputPort())
map = svtk.svtkPolyDataMapper()
map.SetInputConnection(shrink.GetOutputPort())
triangulation = svtk.svtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
