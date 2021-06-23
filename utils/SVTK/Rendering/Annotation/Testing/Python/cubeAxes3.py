#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This example illustrates how one may explicitly specify the range of each
# axes that's used to define the prop, while displaying data with a different
# set of bounds (unlike cubeAxes2.tcl). This example allows you to separate
# the notion of extent of the axes in physical space (bounds) and the extent
# of the values it represents. In other words, you can have the ticks and
# labels show a different range.
#
# read in an interesting object and outline it
#
fohe = svtk.svtkBYUReader()
fohe.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/teapot.g")

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(fohe.GetOutputPort())

foheMapper = svtk.svtkPolyDataMapper()
foheMapper.SetInputConnection(normals.GetOutputPort())

foheActor = svtk.svtkLODActor()
foheActor.SetMapper(foheMapper)
foheActor.GetProperty().SetDiffuseColor(0.7, 0.3, 0.0)

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())

mapOutline = svtk.svtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the RenderWindow, Renderer, and setup viewports
camera = svtk.svtkCamera()
camera.SetClippingRange(1.0, 100.0)
camera.SetFocalPoint(0.9, 1.0, 0.0)
camera.SetPosition(11.63, 6.0, 10.77)

light = svtk.svtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

ren2 = svtk.svtkRenderer()
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren2)
renWin.SetWindowName("SVTK - Cube Axes custom range")

renWin.SetSize(600, 600)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)
ren2.SetBackground(0.1, 0.2, 0.4)

normals.Update()

bounds = normals.GetOutput().GetBounds()
axes2 = svtk.svtkCubeAxesActor()
axes2.SetBounds(
  bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])
axes2.SetXAxisRange(20, 300)
axes2.SetYAxisRange(-0.01, 0.01)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetXLabelFormat("%6.1f")
axes2.SetYLabelFormat("%6.1f")
axes2.SetZLabelFormat("%6.1f")
axes2.SetFlyModeToClosestTriad()
axes2.SetScreenSize(20.0)

ren2.AddViewProp(axes2)

renWin.Render()
ren2.ResetCamera()
renWin.Render()

# render the image
#
iren.Initialize()

def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

#iren.Start()
