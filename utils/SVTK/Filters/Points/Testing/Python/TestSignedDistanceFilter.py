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

# Cut the sphere in half with a plane
plane = svtk.svtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Boolean (intersect) these together to create a hemi-sphere
imp = svtk.svtkImplicitBoolean()
imp.SetOperationTypeToIntersection()
imp.AddFunction(sphere)
imp.AddFunction(plane)

# Extract points along hemi-sphere surface
extract = svtk.svtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.005)
extract.Update()

# Now generate normals from resulting points
norms = svtk.svtkPCANormalEstimation()
norms.SetInputConnection(extract.GetOutputPort())
norms.SetSampleSize(20)
norms.FlipNormalsOff()
norms.SetNormalOrientationToGraphTraversal()
#norms.SetNormalOrientationToPoint()
#norms.SetOrientationPoint(0.3,0.3,0.3)
norms.Update()

subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)

# Generate signed distance function and contour it
dist = svtk.svtkSignedDistance()
dist.SetInputConnection(norms.GetOutputPort())
dist.SetRadius(0.1) #how far out to propagate distance calculation
dist.SetBounds(-1,1, -1,1, -1,1)
dist.SetDimensions(50,50,50)

# Extract the surface with modified flying edges
#fe = svtk.svtkFlyingEdges3D()
#fe.SetValue(0,0.0)
fe = svtk.svtkExtractSurface()
fe.SetInputConnection(dist.GetOutputPort())
fe.SetRadius(0.1) # this should match the signed distance radius

# Time the execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
fe.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate and extract distance function: {0}".format(time))
print("   Resulting bounds: {}".format(fe.GetOutput().GetBounds()))

feMapper = svtk.svtkPolyDataMapper()
feMapper.SetInputConnection(fe.GetOutputPort())

feActor = svtk.svtkActor()
feActor.SetMapper(feMapper)

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
ren0.AddActor(feActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,-1,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
