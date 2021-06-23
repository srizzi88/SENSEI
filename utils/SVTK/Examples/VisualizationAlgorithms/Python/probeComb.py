#!/usr/bin/env python

# This shows how to probe a dataset with a plane. The probed data is
# then contoured.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read data.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# We create three planes and position them in the correct position
# using transform filters. They are then appended together and used as
# a probe.
plane = svtk.svtkPlaneSource()
plane.SetResolution(50, 50)
transP1 = svtk.svtkTransform()
transP1.Translate(3.7, 0.0, 28.37)
transP1.Scale(5, 5, 5)
transP1.RotateY(90)
tpd1 = svtk.svtkTransformPolyDataFilter()
tpd1.SetInputConnection(plane.GetOutputPort())
tpd1.SetTransform(transP1)
outTpd1 = svtk.svtkOutlineFilter()
outTpd1.SetInputConnection(tpd1.GetOutputPort())
mapTpd1 = svtk.svtkPolyDataMapper()
mapTpd1.SetInputConnection(outTpd1.GetOutputPort())
tpd1Actor = svtk.svtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0, 0, 0)

transP2 = svtk.svtkTransform()
transP2.Translate(9.2, 0.0, 31.20)
transP2.Scale(5, 5, 5)
transP2.RotateY(90)
tpd2 = svtk.svtkTransformPolyDataFilter()
tpd2.SetInputConnection(plane.GetOutputPort())
tpd2.SetTransform(transP2)
outTpd2 = svtk.svtkOutlineFilter()
outTpd2.SetInputConnection(tpd2.GetOutputPort())
mapTpd2 = svtk.svtkPolyDataMapper()
mapTpd2.SetInputConnection(outTpd2.GetOutputPort())
tpd2Actor = svtk.svtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0, 0, 0)

transP3 = svtk.svtkTransform()
transP3.Translate(13.27, 0.0, 33.30)
transP3.Scale(5, 5, 5)
transP3.RotateY(90)
tpd3 = svtk.svtkTransformPolyDataFilter()
tpd3.SetInputConnection(plane.GetOutputPort())
tpd3.SetTransform(transP3)
outTpd3 = svtk.svtkOutlineFilter()
outTpd3.SetInputConnection(tpd3.GetOutputPort())
mapTpd3 = svtk.svtkPolyDataMapper()
mapTpd3.SetInputConnection(outTpd3.GetOutputPort())
tpd3Actor = svtk.svtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0, 0, 0)

appendF = svtk.svtkAppendPolyData()
appendF.AddInputConnection(tpd1.GetOutputPort())
appendF.AddInputConnection(tpd2.GetOutputPort())
appendF.AddInputConnection(tpd3.GetOutputPort())

# The svtkProbeFilter takes two inputs. One is a dataset to use as the
# probe geometry (SetInput); the other is the data to probe
# (SetSource). The output dataset structure (geometry and topology) of
# the probe is the same as the structure of the input. The probing
# process generates new data values resampled from the source.
probe = svtk.svtkProbeFilter()
probe.SetInputConnection(appendF.GetOutputPort())
probe.SetSourceData(pl3d_output)

contour = svtk.svtkContourFilter()
contour.SetInputConnection(probe.GetOutputPort())
contour.GenerateValues(50, pl3d_output.GetScalarRange())
contourMapper = svtk.svtkPolyDataMapper()
contourMapper.SetInputConnection(contour.GetOutputPort())
contourMapper.SetScalarRange(pl3d_output.GetScalarRange())
planeActor = svtk.svtkActor()
planeActor.SetMapper(contourMapper)

outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.AddActor(tpd1Actor)
ren.AddActor(tpd2Actor)
ren.AddActor(tpd3Actor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 400)

ren.ResetCamera()
cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam1.SetPosition(-12.3332, 31.7479, 41.2387)
cam1.SetViewUp(0.060772, -0.319905, 0.945498)

iren.Initialize()
renWin.Render()
iren.Start()
