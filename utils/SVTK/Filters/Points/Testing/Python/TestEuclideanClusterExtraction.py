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
pts.SetDistance(0.025)
pts.Update()

# Now see if we can extract the three objects as separate clusters.
extr = svtk.svtkEuclideanClusterExtraction()
extr.SetInputConnection(pts.GetOutputPort())
extr.SetRadius(0.1)
extr.ColorClustersOn()
extr.SetExtractionModeToAllClusters()

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
extr.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(pts.GetOutput().GetNumberOfPoints()))
print("   Time to segment objects: {0}".format(time))
print("   Number of clusters: {0}".format(extr.GetNumberOfExtractedClusters()))

# Three different outputs for different curvatures
subMapper = svtk.svtkPointGaussianMapper()
subMapper.SetInputConnection(extr.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)
subMapper.SetScalarRange(0,2)

subActor = svtk.svtkActor()
subActor.SetMapper(subMapper)
subActor.AddPosition(0,2.25,0)

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
