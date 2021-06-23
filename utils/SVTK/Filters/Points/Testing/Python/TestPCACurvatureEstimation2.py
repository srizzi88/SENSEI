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
points.SetBounds(-3,3, -1,1, -1,1)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Create a cylinder
cyl = svtk.svtkCylinder()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)

# Create a (thin) box implicit function
box = svtk.svtkBox()
box.SetBounds(-1,0.5, -0.5,0.5, -0.0005, 0.0005)

# Create a sphere implicit function
sphere = svtk.svtkSphere()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)

# Boolean (union) these together
imp = svtk.svtkImplicitBoolean()
imp.SetOperationTypeToUnion()
imp.AddFunction(cyl)
imp.AddFunction(box)
imp.AddFunction(sphere)

# Extract points along sphere surface
extract = svtk.svtkFitImplicitFunction()
extract.SetInputConnection(points.GetOutputPort())
extract.SetImplicitFunction(imp)
extract.SetThreshold(0.0005)
extract.Update()

# Now generate normals from resulting points
curv = svtk.svtkPCACurvatureEstimation()
curv.SetInputConnection(extract.GetOutputPort())
curv.SetSampleSize(6)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
curv.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to generate curvature: {0}".format(time))

# Break out the curvature into thress separate arrays
assign = svtk.svtkAssignAttribute()
assign.SetInputConnection(curv.GetOutputPort())
assign.Assign("PCACurvature", "VECTORS", "POINT_DATA")

extract = svtk.svtkExtractVectorComponents()
extract.SetInputConnection(assign.GetOutputPort())
extract.Update()
print(extract.GetOutput(0).GetScalarRange())
print(extract.GetOutput(1).GetScalarRange())
print(extract.GetOutput(2).GetScalarRange())

# Three different outputs for different curvatures
subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(extract.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)
subActor.AddPosition(0,2.25,0)

sub1Mapper = svtk.svtkPointGaussianMapper()
sub1Mapper.SetInputConnection(extract.GetOutputPort(1))
sub1Mapper.EmissiveOff()
sub1Mapper.SetScaleFactor(0.0)

sub1Actor = svtk.svtkActor()
sub1Actor.SetMapper(sub1Mapper)

sub2Mapper = svtk.svtkPointGaussianMapper()
sub2Mapper.SetInputConnection(extract.GetOutputPort(2))
sub2Mapper.EmissiveOff()
sub2Mapper.SetScaleFactor(0.0)

sub2Actor = svtk.svtkActor()
sub2Actor.SetMapper(sub2Mapper)
sub2Actor.AddPosition(0,-2.25,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(sub1Actor)
ren0.AddActor(sub2Actor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
