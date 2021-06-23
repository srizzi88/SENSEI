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
sphere = svtk.svtkSphereSource()
sphere.SetPhiResolution(15)
sphere.SetThetaResolution(30)
plane = svtk.svtkPlane()
plane.SetNormal(1,0,0)
cut = svtk.svtkCutter()
cut.SetInputConnection(sphere.GetOutputPort())
cut.SetCutFunction(plane)
cut.GenerateCutScalarsOn()
strip = svtk.svtkStripper()
strip.SetInputConnection(cut.GetOutputPort())
points = svtk.svtkPoints()
points.InsertPoint(0,1,0,0)
lines = svtk.svtkCellArray()
lines.InsertNextCell(2)
#number of points
lines.InsertCellPoint(0)
lines.InsertCellPoint(0)
tip = svtk.svtkPolyData()
tip.SetPoints(points)
tip.SetLines(lines)
appendPD = svtk.svtkAppendPolyData()
appendPD.AddInputConnection(strip.GetOutputPort())
appendPD.AddInputData(tip)
# extrude profile to make coverage
#
extrude = svtk.svtkRuledSurfaceFilter()
extrude.SetInputConnection(appendPD.GetOutputPort())
extrude.SetRuledModeToPointWalk()
clean = svtk.svtkCleanPolyData()
clean.SetInputConnection(extrude.GetOutputPort())
clean.ConvertPolysToLinesOff()
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(clean.GetOutputPort())
mapper.ScalarVisibilityOff()
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(.4)
ren1.AddActor(actor)
renWin.SetSize(200,200)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
