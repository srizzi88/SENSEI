#!/usr/bin/env python

# Demonstrate how to use the svtkBoxWidget 3D widget. This script uses
# a 3D box widget to define a "clipping box" to clip some simple
# geometry (a mace). Make sure that you hit the "i" key to activate
# the widget.

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

# The sphere and spikes are appended into a single polydata. This just
# makes things simpler to manage.
apd = svtk.svtkAppendPolyData()
apd.AddInputConnection(glyph.GetOutputPort())
apd.AddInputConnection(sphere.GetOutputPort())
maceMapper = svtk.svtkPolyDataMapper()
maceMapper.SetInputConnection(apd.GetOutputPort())
maceActor = svtk.svtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

# This portion of the code clips the mace with the svtkPlanes implicit
# function.  The clipped region is colored green.
planes = svtk.svtkPlanes()
clipper = svtk.svtkClipPolyData()
clipper.SetInputConnection(apd.GetOutputPort())
clipper.SetClipFunction(planes)
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

# The SetInteractor method is how 3D widgets are associated with the
# render window interactor.  Internally, SetInteractor sets up a bunch
# of callbacks using the Command/Observer mechanism (AddObserver()).
boxWidget = svtk.svtkBoxWidget()
boxWidget.SetInteractor(iren)
boxWidget.SetPlaceFactor(1.25)

ren.AddActor(maceActor)
ren.AddActor(selectActor)

# Add the actors to the renderer, set the background and size
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

# This callback function does the actual work: updates the svtkPlanes
# implicit function.  This in turn causes the pipeline to update.
def SelectPolygons(object, event):
    # object will be the boxWidget
    global selectActor, planes
    object.GetPlanes(planes)
    selectActor.VisibilityOn()

glyph.Update()
# Place the interactor initially. The input to a 3D widget is used to
# initially position and scale the widget. The "EndInteractionEvent" is
# observed which invokes the SelectPolygons callback.
boxWidget.SetInputConnection(glyph.GetOutputPort())
boxWidget.PlaceWidget()
boxWidget.AddObserver("EndInteractionEvent", SelectPolygons)

iren.Initialize()
renWin.Render()
iren.Start()
