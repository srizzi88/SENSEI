#!/usr/bin/env python
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Control debugging parameters
res = 50

# Create the RenderWindow, Renderers
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = svtk.svtkSphere()
sphere.SetCenter(0.0, 0.0, 0.0)
sphere.SetRadius(0.25)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
sample.SetSampleDimensions(res, res, res)
sample.Update()

# Converts image data to structured grid
convert = svtk.svtkImageDataToPointSet()
convert.SetInputConnection(sample.GetOutputPort())
convert.Update()

cthvtr = svtk.svtkXMLRectilinearGridReader()
cthvtr.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/cth.vtr")
cthvtr.CellArrayStatus = ['Pressure', 'Void Volume Fraction', 'X Velocity', 'Y Velocity', 'Z Velocity', 'Volume Fraction for Armor Plate', 'Mass for Armor Plate', 'Volume Fraction for Body, Nose', 'Mass for Body, Nose']
cthvtr.Update()
input = cthvtr.GetOutput()

# Create a cutting plane
plane = svtk.svtkPlane()
plane.SetOrigin(input.GetCenter())
plane.SetNormal(1, 1, 1)

# First create the usual cutter
cutter = svtk.svtkCutter()
cutter.SetInputData(input)
cutter.SetCutFunction(plane)

cutterMapper = svtk.svtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

cutterActor = svtk.svtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1, 1, 1)

# Throw in an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputData(input)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the accelerated version.
sCutter = svtk.svtkPlaneCutter()
sCutter.SetInputData(input)
sCutter.SetPlane(plane)

sCutterMapper = svtk.svtkCompositePolyDataMapper()
sCutterMapper.SetInputConnection(sCutter.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = svtk.svtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1, 1, 1)

# Accelerated cutter without tree
snCutter = svtk.svtkPlaneCutter()
snCutter.SetInputData(input)
snCutter.SetPlane(plane)
snCutter.BuildTreeOff()

snCutterMapper = svtk.svtkCompositePolyDataMapper()
snCutterMapper.SetInputConnection(snCutter.GetOutputPort())
snCutterMapper.ScalarVisibilityOff()

snCutterActor = svtk.svtkActor()
snCutterActor.SetMapper(snCutterMapper)
snCutterActor.GetProperty().SetColor(1,1,1)

outlineT = svtk.svtkOutlineFilter()
outlineT.SetInputData(input)

outlineMapperT = svtk.svtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = svtk.svtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the filter w/out scalar tree
cutter_timer = svtk.svtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("svtkCutter:", CT)

# Time the execution of the filter w/ sphere tree
sCutter_timer = svtk.svtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
sCutter.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print ("Build sphere tree + execute once:", ST)

sCutter_timer = svtk.svtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
plane.Modified()
sCutter.Update()
SC = sCutter_timer.GetElapsedWallClockTime()
print ("svtkPlaneCutter:", SC)

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(outlineActorT)
ren1.AddActor(sCutterActor)
ren2.AddActor(outlineActorT)
ren2.AddActor(snCutterActor)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)
ren0.SetViewport(0,0,0.33,1);
ren1.SetViewport(0.33,0,0.66,1);
ren2.SetViewport(0.66,0,1,1);
renWin.SetSize(900,300)
ren0.ResetCamera()
ren1.ResetCamera()
ren2.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
