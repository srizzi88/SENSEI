#!/usr/bin/env python

# This example demonstrates the use of svtkCubeAxesActor2D to indicate
# the position in space that the camera is currently viewing.  The
# svtkCubeAxesActor2D draws axes on the bounding box of the data set
# and labels the axes with x-y-z coordinates.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create a svtkBYUReader and read in a data set.
fohe = svtk.svtkBYUReader()
fohe.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/teapot.g")

# Create a svtkPolyDataNormals filter to calculate the normals of the
# data set.
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(fohe.GetOutputPort())
# Set up the associated mapper and actor.
foheMapper = svtk.svtkPolyDataMapper()
foheMapper.SetInputConnection(normals.GetOutputPort())
foheActor = svtk.svtkLODActor()
foheActor.SetMapper(foheMapper)

# Create a svtkOutlineFilter to draw the bounding box of the data set.
# Also create the associated mapper and actor.
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())
mapOutline = svtk.svtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create a svtkCamera, and set the camera parameters.
camera = svtk.svtkCamera()
camera.SetClippingRange(1.60187, 20.0842)
camera.SetFocalPoint(0.21406, 1.5, 0)
camera.SetPosition(8.3761, 4.94858, 4.12505)
camera.SetViewUp(0.180325, 0.549245, -0.815974)

# Create a svtkLight, and set the light parameters.
light = svtk.svtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

# Create the Renderers.  Assign them the appropriate viewport
# coordinates, active camera, and light.
ren = svtk.svtkRenderer()
ren.SetViewport(0, 0, 0.5, 1.0)
ren.SetActiveCamera(camera)
ren.AddLight(light)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.5, 0, 1.0, 1.0)
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

# Create the RenderWindow and RenderWindowInteractor.
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.AddRenderer(ren2)
renWin.SetWindowName("SVTK - Cube Axes")
renWin.SetSize(600, 300)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, and set the background.
ren.AddViewProp(foheActor)
ren.AddViewProp(outlineActor)
ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)

ren.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)

# Create a text property for both cube axes
tprop = svtk.svtkTextProperty()
tprop.SetColor(1, 1, 1)
tprop.ShadowOn()

# Create a svtkCubeAxesActor2D.  Use the outer edges of the bounding box to
# draw the axes.  Add the actor to the renderer.
axes = svtk.svtkCubeAxesActor2D()
axes.SetInputConnection(normals.GetOutputPort())
axes.SetCamera(ren.GetActiveCamera())
axes.SetLabelFormat("%6.4g")
axes.SetFlyModeToOuterEdges()
axes.SetFontFactor(0.8)
axes.SetAxisTitleTextProperty(tprop)
axes.SetAxisLabelTextProperty(tprop)
ren.AddViewProp(axes)

# Create a svtkCubeAxesActor2D.  Use the closest vertex to the camera to
# determine where to draw the axes.  Add the actor to the renderer.
axes2 = svtk.svtkCubeAxesActor2D()
axes2.SetViewProp(foheActor)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetLabelFormat("%6.4g")
axes2.SetFlyModeToClosestTriad()
axes2.SetFontFactor(0.8)
axes2.ScalingOff()
axes2.SetAxisTitleTextProperty(tprop)
axes2.SetAxisLabelTextProperty(tprop)
ren2.AddViewProp(axes2)

# Set up a check for aborting rendering.
def CheckAbort(obj, event):
    # obj will be the object generating the event.  In this case it
    # is renWin.
    if obj.GetEventPending() != 0:
        obj.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", CheckAbort)

iren.Initialize()
renWin.Render()
iren.Start()
