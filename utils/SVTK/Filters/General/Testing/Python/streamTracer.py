#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
# this test has some wireframe geometry
# Make sure multisampling is disabled to avoid generating multiple
# regression images
# renWin SetMultiSamples 0
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
reader = svtk.svtkStructuredGridReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/office.binary.svtk")
reader.Update()
#force a read to occur
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
mapOutline = svtk.svtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
rk = svtk.svtkRungeKutta45()
# Create source for streamtubes
streamer = svtk.svtkStreamTracer()
streamer.SetInputConnection(reader.GetOutputPort())
streamer.SetStartPosition(0.1,2.1,0.5)
streamer.SetMaximumPropagation(500)
streamer.SetIntegrationStepUnit(2)
streamer.SetMinimumIntegrationStep(0.1)
streamer.SetMaximumIntegrationStep(1.0)
streamer.SetInitialIntegrationStep(0.2)
streamer.SetIntegrationDirection(0)
streamer.SetIntegrator(rk)
streamer.SetRotationScale(0.5)
streamer.SetMaximumError(1.0e-8)
aa = svtk.svtkAssignAttribute()
aa.SetInputConnection(streamer.GetOutputPort())
aa.Assign("Normals","NORMALS","POINT_DATA")
rf1 = svtk.svtkRibbonFilter()
rf1.SetInputConnection(aa.GetOutputPort())
rf1.SetWidth(0.1)
rf1.VaryWidthOff()
mapStream = svtk.svtkPolyDataMapper()
mapStream.SetInputConnection(rf1.GetOutputPort())
mapStream.SetScalarRange(reader.GetOutput().GetScalarRange())
streamActor = svtk.svtkActor()
streamActor.SetMapper(mapStream)
ren1.AddActor(outlineActor)
ren1.AddActor(streamActor)
ren1.SetBackground(0.4,0.4,0.5)
cam = ren1.GetActiveCamera()
cam.SetPosition(-2.35599,-3.35001,4.59236)
cam.SetFocalPoint(2.255,2.255,1.28413)
cam.SetViewUp(0.311311,0.279912,0.908149)
cam.SetClippingRange(1.12294,16.6226)
renWin.SetSize(300,200)
iren.Initialize()
# interact with data
iren.Start()
# --- end of script --
