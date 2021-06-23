#!/usr/bin/env python

# This example demonstrates the use of 2D text the old way by using a
# svtkTextMapper and a svtkScaledTextActor.

import svtk

# Create a sphere source, mapper, and actor
sphere = svtk.svtkSphereSource()

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = svtk.svtkLODActor()
sphereActor.SetMapper(sphereMapper)

# Create a text mapper.
textMapper = svtk.svtkTextMapper()
textMapper.SetInput("This is a sphere")

# Set the text, font, justification, and text properties (bold,
# italics, etc.).
tprop = textMapper.GetTextProperty()
tprop.SetFontSize(18)
tprop.SetFontFamilyToArial()
tprop.SetJustificationToCentered()
tprop.BoldOn()
tprop.ItalicOn()
tprop.ShadowOn()
tprop.SetColor(0, 0, 1)

# Create a scaled text actor. Set the position of the text.
textActor = svtk.svtkScaledTextActor()
textActor.SetMapper(textMapper)
textActor.SetDisplayPosition(90, 50)

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
