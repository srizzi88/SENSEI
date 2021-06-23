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
# create room profile
#
points = svtk.svtkPoints()
points.InsertPoint(0,0,0,0)
points.InsertPoint(1,1,0,0)
points.InsertPoint(2,1,1,0)
points.InsertPoint(3,2,1,0)
lines = svtk.svtkCellArray()
lines.InsertNextCell(4)
#number of points
lines.InsertCellPoint(0)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)
lines.InsertCellPoint(3)
profile = svtk.svtkPolyData()
profile.SetPoints(points)
profile.SetLines(lines)
xfm = svtk.svtkTransform()
xfm.Translate(0,0,8)
xfm.RotateZ(90)
xfmPd = svtk.svtkTransformPolyDataFilter()
xfmPd.SetInputData(profile)
xfmPd.SetTransform(xfm)
appendPD = svtk.svtkAppendPolyData()
appendPD.AddInputData(profile)
appendPD.AddInputConnection(xfmPd.GetOutputPort())
# extrude profile to make wall
#
extrude = svtk.svtkRuledSurfaceFilter()
extrude.SetInputConnection(appendPD.GetOutputPort())
extrude.SetResolution(51,51)
extrude.SetRuledModeToResample()
map = svtk.svtkPolyDataMapper()
map.SetInputConnection(extrude.GetOutputPort())
wall = svtk.svtkActor()
wall.SetMapper(map)
wall.GetProperty().SetColor(0.3800,0.7000,0.1600)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(wall)
ren1.SetBackground(1,1,1)
renWin.SetSize(200,200)
ren1.GetActiveCamera().SetPosition(12.9841,-1.81551,8.82706)
ren1.GetActiveCamera().SetFocalPoint(0.5,1,4)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0.128644,-0.675064,-0.726456)
ren1.GetActiveCamera().SetClippingRange(7.59758,21.3643)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
