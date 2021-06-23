#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Parameters for testing
sze = 300

# Graphics stuff
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = svtk.svtkRenderWindow()
renWin.SetSize(2*sze+100,sze)
#renWin.SetSize(2*sze,sze)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline, render simple object. We'll also color
# the plane.
plane = svtk.svtkPlaneSource()
plane.SetOrigin(0,0,0)
plane.SetPoint1(2,0,0)
plane.SetPoint2(0,1,0)

ele = svtk.svtkElevationFilter()
ele.SetInputConnection(plane.GetOutputPort())
ele.SetLowPoint(0,0,0)
ele.SetHighPoint(0,1,0)

planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(ele.GetOutputPort())

planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)

ren0.AddActor(planeActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

# Extract rendered geometry, convert to point cloud
renSource = svtk.svtkRendererSource()
renSource.SetInput(ren0)
renSource.WholeWindowOff()
renSource.DepthValuesOn()
renSource.Update()

pc = svtk.svtkDepthImageToPointCloud()
pc.SetInputConnection(renSource.GetOutputPort())
pc.SetCamera(ren0.GetActiveCamera())
pc.CullFarPointsOff()

timer = svtk.svtkTimerLog()
timer.StartTimer()
pc.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Generate point cloud: {0}".format(time))

pcMapper = svtk.svtkPolyDataMapper()
pcMapper.SetInputConnection(pc.GetOutputPort())

pcActor = svtk.svtkActor()
pcActor.SetMapper(pcMapper)

ren1.AddActor(pcActor)
ren1.SetBackground(0,0,0)
cam = ren1.GetActiveCamera()
cam.SetFocalPoint(0,0,0)
cam.SetPosition(1,1,1)
ren1.ResetCamera()

renWin.Render()
#iren.Start()
