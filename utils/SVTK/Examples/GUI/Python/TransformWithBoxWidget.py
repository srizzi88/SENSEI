#!/usr/bin/env python

# Demonstrate how to use the svtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import svtk

# Start by creating some simple geometry; in this case a mace.
sphere = svtk.svtkSphereSource()
cone = svtk.svtkConeSource()
glyph = svtk.svtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
appendData = svtk.svtkAppendPolyData()
appendData.AddInputConnection(glyph.GetOutputPort())
appendData.AddInputConnection(sphere.GetOutputPort())
maceMapper = svtk.svtkPolyDataMapper()
maceMapper.SetInputConnection(appendData.GetOutputPort())
maceActor = svtk.svtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# The box widget observes the events invoked by the render window
# interactor.  These events come from user interaction in the render
# window.
boxWidget = svtk.svtkBoxWidget()
boxWidget.SetInteractor(iren)
boxWidget.SetPlaceFactor(1.25)

# Add the actors to the renderer, set the background and window size.
ren.AddActor(maceActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

# As the box widget is interacted with, it produces a transformation
# matrix that is set on the actor.
t = svtk.svtkTransform()
def TransformActor(obj, event):
    global t, maceActor
    obj.GetTransform(t)
    maceActor.SetUserTransform(t)

# Place the interactor initially. The actor is used to place and scale
# the interactor. An observer is added to the box widget to watch for
# interaction events. This event is captured and used to set the
# transformation matrix of the actor.
boxWidget.SetProp3D(maceActor)
boxWidget.PlaceWidget()
boxWidget.AddObserver("InteractionEvent", TransformActor)

iren.Initialize()
renWin.Render()
iren.Start()
