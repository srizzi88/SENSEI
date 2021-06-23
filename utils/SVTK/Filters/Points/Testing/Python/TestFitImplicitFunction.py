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

# Create a sphere implicit function
sphere = svtk.svtkSphere()
sphere.SetCenter(0.9,0.1,0.1)
sphere.SetRadius(0.33)

# Extract points within sphere
extract = svtk.svtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
extract.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to extract points: {0}".format(time))
print("   Number removed: {0}".format(extract.GetNumberOfPointsRemoved()))
print("   Original number of points: {0}".format(NPts))

# First output are the non-outliers
extMapper = svtk.svtkPointGaussianMapper()
extMapper.SetInputConnection(extract.GetOutputPort())
extMapper.EmissiveOff()
extMapper.SetScaleFactor(0.0)

extActor = svtk.svtkActor()
extActor.SetMapper(extMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(extActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

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
