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
sphere.SetCenter(0,0,0)
sphere.SetRadius(0.75)

# Extract points along sphere surface
extract = svtk.svtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(sphere)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = svtk.svtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOn()
norms.SetNormalOrientationToGraphTraversal()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
norms.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate normals: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(norms.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)

# Draw the normals
mask = svtk.svtkMaskPoints()
mask.SetInputConnection(norms.GetOutputPort())
mask.SetRandomModeType(1)
mask.SetMaximumNumberOfPoints(250)

hhog = svtk.svtkHedgeHog()
hhog.SetInputConnection(mask.GetOutputPort())
hhog.SetVectorModeToUseNormal()
hhog.SetScaleFactor(0.25)

hogMapper = svtk.svtkPolyDataMapper()
hogMapper.SetInputConnection(hhog.GetOutputPort())

hogActor = svtk.svtkActor()
hogActor.SetMapper(hogMapper)

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
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(hogActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
