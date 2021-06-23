#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 10000
math = svtk.svtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = svtk.svtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Reuse the locator
locator = svtk.svtkStaticPointLocator()
locator.SetDataSet(points.GetOutput())
locator.BuildLocator()

# Remove isolated points
removal = svtk.svtkRadiusOutlierRemoval()
removal.SetInputConnection(points.GetOutputPort())
removal.SetLocator(locator)
removal.SetRadius(0.1)
removal.SetNumberOfNeighbors(2)
removal.GenerateOutliersOn()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
removal.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to remove points: {0}".format(time))
print("   Number removed: {0}".format(removal.GetNumberOfPointsRemoved()))
print("   Original number of points: {0}".format(NPts))

# First output are the non-outliers
remMapper = svtk.svtkPointGaussianMapper()
remMapper.SetInputConnection(removal.GetOutputPort())
remMapper.EmissiveOff()
remMapper.SetScaleFactor(0.0)

remActor = svtk.svtkActor()
remActor.SetMapper(remMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Second output are the outliers
remMapper1 = svtk.svtkPointGaussianMapper()
remMapper1.SetInputConnection(removal.GetOutputPort(1))
remMapper1.EmissiveOff()
remMapper1.SetScaleFactor(0.0)

remActor1 = svtk.svtkActor()
remActor1.SetMapper(remMapper1)

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
ren0.AddActor(remActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(remActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

ren1.SetActiveCamera(cam)

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
