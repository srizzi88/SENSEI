#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline
#

# Create a cylinder
cyl = svtk.svtkCylinderSource()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)
cyl.SetHeight(1.8)
cyl.SetResolution(24)

# Create a (thin) box implicit function
plane = svtk.svtkPlaneSource()
plane.SetOrigin(-1, -0.5, 0)
plane.SetPoint1(0.5, -0.5, 0)
plane.SetPoint2(-1, 0.5, 0)

# Create a sphere implicit function
sphere = svtk.svtkSphereSource()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)
sphere.SetThetaResolution(96)
sphere.SetPhiResolution(48)

# Boolean (union) these together
append = svtk.svtkAppendPolyData()
append.AddInputConnection(cyl.GetOutputPort())
append.AddInputConnection(plane.GetOutputPort())
append.AddInputConnection(sphere.GetOutputPort())

# Extract points along sphere surface
pts = svtk.svtkPolyDataPointSampler()
pts.SetInputConnection(append.GetOutputPort())
pts.SetDistance(0.01)
pts.Update()

# Now generate normals from resulting points
curv = svtk.svtkPCACurvatureEstimation()
curv.SetInputConnection(pts.GetOutputPort())
curv.SetSampleSize(20)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
curv.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(pts.GetOutput().GetNumberOfPoints()))
print("   Time to generate curvature: {0}".format(time))

# Break out the curvature into three separate arrays
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
