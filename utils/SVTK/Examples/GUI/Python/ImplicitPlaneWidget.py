#!/usr/bin/env python

# This example demonstrates how to use the svtkPlaneWidget to probe a
# dataset and then generate contours on the probed data.

import svtk

# Create a mace out of filters.
sphere = svtk.svtkSphereSource()
cone = svtk.svtkConeSource()
glyph = svtk.svtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

# The sphere and spikes are appended into a single polydata.
# This just makes things simpler to manage.
apd = svtk.svtkAppendPolyData()
apd.AddInputConnection(glyph.GetOutputPort())
apd.AddInputConnection(sphere.GetOutputPort())

maceMapper = svtk.svtkPolyDataMapper()
maceMapper.SetInputConnection(apd.GetOutputPort())

maceActor = svtk.svtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

# This portion of the code clips the mace with the svtkPlanes
# implicit function. The clipped region is colored green.
plane = svtk.svtkPlane()
clipper = svtk.svtkClipPolyData()
clipper.SetInputConnection(apd.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.InsideOutOn()

selectMapper = svtk.svtkPolyDataMapper()
selectMapper.SetInputConnection(clipper.GetOutputPort())

selectActor = svtk.svtkLODActor()
selectActor.SetMapper(selectMapper)
selectActor.GetProperty().SetColor(0, 1, 0)
selectActor.VisibilityOff()
selectActor.SetScale(1.01, 1.01, 1.01)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# The callback function
def myCallback(obj, event):
    global plane, selectActor
    obj.GetPlane(plane)
    selectActor.VisibilityOn()

# Associate the line widget with the interactor
planeWidget = svtk.svtkImplicitPlaneWidget()
planeWidget.SetInteractor(iren)
planeWidget.SetPlaceFactor(1.25)
planeWidget.SetInputConnection(glyph.GetOutputPort())
planeWidget.PlaceWidget()
planeWidget.AddObserver("InteractionEvent", myCallback)

ren.AddActor(maceActor)
ren.AddActor(selectActor)

# Add the actors to the renderer, set the background and size
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

# Start interaction.
iren.Initialize()
renWin.Render()
iren.Start()
