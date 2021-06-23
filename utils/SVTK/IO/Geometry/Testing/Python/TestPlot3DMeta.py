#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

renWin = svtk.svtkRenderWindow()

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

r = svtk.svtkPlot3DMetaReader()
r.SetFileName("%s/Data/test.p3d" % SVTK_DATA_ROOT)

r.UpdateInformation()
outInfo = r.GetOutputInformation(0)
l = outInfo.Length(svtk.svtkStreamingDemandDrivenPipeline.TIME_STEPS())
if l != 2:
    raise "Error: wrong number of time steps: %d. Should be 2" % l

outInfo.Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 3.5)
r.Update()

outInfo.Set(svtk.svtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 4.5)
r.Update()

output = r.GetOutput().GetBlock(0)

plane = svtk.svtkStructuredGridGeometryFilter()
plane.SetInputData(output)
plane.SetExtent(25,25,0,100,0,100)

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(plane.GetOutputPort())
mapper.SetScalarRange(output.GetPointData().GetScalars().GetRange())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren = svtk.svtkRenderer()

camera = svtk.svtkCamera()
ren.SetActiveCamera(camera)

renWin.AddRenderer(ren)
ren.AddActor(actor)

camera.SetViewUp(0,1,0)
camera.SetFocalPoint(0,0,0)
camera.SetPosition(1,0,0)
ren.ResetCamera()
camera.Dolly(1.25)
ren.ResetCameraClippingRange()

renWin.SetSize(400,300)
renWin.Render()

iren.Initialize()
