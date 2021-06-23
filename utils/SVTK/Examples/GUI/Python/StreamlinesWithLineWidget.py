#!/usr/bin/env python

# This example demonstrates how to use the svtkLineWidget to seed and
# manipulate streamlines. Two line widgets are created. One is invoked
# by pressing 'i', the other by pressing 'L' (capital). Both can exist
# together.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Start by loading some data.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# The line widget is used seed the streamlines.
lineWidget = svtk.svtkLineWidget()
seeds = svtk.svtkPolyData()
lineWidget.SetInputData(pl3d_output)
lineWidget.SetAlignToYAxis()
lineWidget.PlaceWidget()
lineWidget.GetPolyData(seeds)
lineWidget.ClampToBoundsOn()

rk4 = svtk.svtkRungeKutta4()
streamer = svtk.svtkStreamTracer()
streamer.SetInputData(pl3d_output)
streamer.SetSourceData(seeds)
streamer.SetMaximumPropagation(100)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetComputeVorticity(1)
streamer.SetIntegrator(rk4)
rf = svtk.svtkRibbonFilter()
rf.SetInputConnection(streamer.GetOutputPort())
rf.SetWidth(0.1)
rf.SetWidthFactor(5)
streamMapper = svtk.svtkPolyDataMapper()
streamMapper.SetInputConnection(rf.GetOutputPort())
streamMapper.SetScalarRange(pl3d_output.GetScalarRange())
streamline = svtk.svtkActor()
streamline.SetMapper(streamMapper)
streamline.VisibilityOff()

# The second line widget is used seed more streamlines.
lineWidget2 = svtk.svtkLineWidget()
seeds2 = svtk.svtkPolyData()
lineWidget2.SetInputData(pl3d_output)
lineWidget2.PlaceWidget()
lineWidget2.GetPolyData(seeds2)
lineWidget2.SetKeyPressActivationValue('L')

streamer2 = svtk.svtkStreamTracer()
streamer2.SetInputData(pl3d_output)
streamer2.SetSourceData(seeds2)
streamer2.SetMaximumPropagation(100)
streamer2.SetInitialIntegrationStep(.2)
streamer2.SetIntegrationDirectionToForward()
streamer2.SetComputeVorticity(1)
streamer2.SetIntegrator(rk4)
rf2 = svtk.svtkRibbonFilter()
rf2.SetInputConnection(streamer2.GetOutputPort())
rf2.SetWidth(0.1)
rf2.SetWidthFactor(5)
streamMapper2 = svtk.svtkPolyDataMapper()
streamMapper2.SetInputConnection(rf2.GetOutputPort())
streamMapper2.SetScalarRange(pl3d_output.GetScalarRange())
streamline2 = svtk.svtkActor()
streamline2.SetMapper(streamMapper2)
streamline2.VisibilityOff()

outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Callback functions that actually generate streamlines.
def BeginInteraction(obj, event):
    global streamline
    streamline.VisibilityOn()

def GenerateStreamlines(obj, event):
    global seeds, renWin
    obj.GetPolyData(seeds)
    renWin.Render()

def BeginInteraction2(obj, event):
    global streamline2
    streamline2.VisibilityOn()

def GenerateStreamlines2(obj, event):
    global seeds2, renWin
    obj.GetPolyData(seeds2)
    renWin.Render()

# Associate the line widget with the interactor and setup callbacks.
lineWidget.SetInteractor(iren)
lineWidget.AddObserver("StartInteractionEvent", BeginInteraction)
lineWidget.AddObserver("InteractionEvent", GenerateStreamlines)

lineWidget2.SetInteractor(iren)
lineWidget2.AddObserver("StartInteractionEvent", BeginInteraction2)
lineWidget2.AddObserver("EndInteractionEvent", GenerateStreamlines2)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(streamline)
ren.AddActor(streamline2)

ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

iren.Initialize()
renWin.Render()
iren.Start()
