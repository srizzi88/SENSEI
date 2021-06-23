#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = svtk.svtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Subsample
subsample = svtk.svtkVoxelGrid()
subsample.SetInputConnection(points.GetOutputPort())
subsample.SetConfigurationStyleToManual()
subsample.SetDivisions(47,47,47)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
subsample.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to subsample: {0}".format(time))
print("   Number of divisions: {}".format(subsample.GetDivisions()))
print("   Original number of points: {0}".format(NPts))
print("   Final number of points: {0}".format(subsample.GetOutput().GetNumberOfPoints()))

# Output the original points
subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(points.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Output the subsampled points
subMapper1 = svtk.svtkPointGaussianMapper()
subMapper1.SetInputConnection(subsample.GetOutputPort())
subMapper1.EmissiveOff()
subMapper1.SetScaleFactor(0.0)

subActor1 = svtk.svtkActor()
subActor1.SetMapper(subMapper1)

# Create an outline
outline1 = svtk.svtkOutlineFilter()
outline1.SetInputConnection(points.GetOutputPort())

outlineMapper1 = svtk.svtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = svtk.svtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,.5,1)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(subActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
