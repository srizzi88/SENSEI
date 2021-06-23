#!/usr/bin/env python

# This example demonstrates the use of 2D text.

import svtk

# Create a sphere source, mapper, and actor
sphere = svtk.svtkSphereSource()

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = svtk.svtkLODActor()
sphereActor.SetMapper(sphereMapper)

# Create a scaled text actor.
# Set the text, font, justification, and properties (bold, italics,
# etc.).
textActor = svtk.svtkTextActor()
textActor.SetTextScaleModeToProp()
textActor.SetDisplayPosition(90, 50)
textActor.SetInput("This is a sphere")

# Set coordinates to match the old svtkScaledTextActor default value
textActor.GetPosition2Coordinate().SetCoordinateSystemToNormalizedViewport()
textActor.GetPosition2Coordinate().SetValue(0.6, 0.1)

tprop = textActor.GetTextProperty()
tprop.SetFontSize(18)
tprop.SetFontFamilyToArial()
tprop.SetJustificationToCentered()
tprop.BoldOn()
tprop.ItalicOn()
tprop.ShadowOn()
tprop.SetColor(0, 0, 1)

# Create the Renderer, RenderWindow, RenderWindowInteractor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer; set the background and size; zoom
# in; and render.
ren.AddActor2D(textActor)
ren.AddActor(sphereActor)

ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 125)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
