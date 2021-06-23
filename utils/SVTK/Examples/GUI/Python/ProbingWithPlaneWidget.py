#!/usr/bin/env python

# This example demonstrates how to use the svtkPlaneWidget to probe
# a dataset and then generate contours on the probed data.

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

# The plane widget is used probe the dataset.
planeWidget = svtk.svtkPlaneWidget()
planeWidget.SetInputData(pl3d_output)
planeWidget.NormalToXAxisOn()
planeWidget.SetResolution(20)
planeWidget.SetRepresentationToOutline()
planeWidget.PlaceWidget()
plane = svtk.svtkPolyData()
planeWidget.GetPolyData(plane)

probe = svtk.svtkProbeFilter()
probe.SetInputData(plane)
probe.SetSourceData(pl3d_output)

contourMapper = svtk.svtkPolyDataMapper()
contourMapper.SetInputConnection(probe.GetOutputPort())
contourMapper.SetScalarRange(pl3d_output.GetScalarRange())
contourActor = svtk.svtkActor()
contourActor.SetMapper(contourMapper)
contourActor.VisibilityOff()

# An outline is shown for context.
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

# Actually generate contour lines.
def BeginInteraction(obj, event):
    global plane, contourActor
    obj.GetPolyData(plane)
    contourActor.VisibilityOn()

def ProbeData(obj, event):
    global plane
    obj.GetPolyData(plane)


# Associate the widget with the interactor
planeWidget.SetInteractor(iren)
# Handle the events.
planeWidget.AddObserver("EnableEvent", BeginInteraction)
planeWidget.AddObserver("StartInteractionEvent", BeginInteraction)
planeWidget.AddObserver("InteractionEvent", ProbeData)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(contourActor)

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
