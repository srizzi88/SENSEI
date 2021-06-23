#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = svtk.svtkSphereSource()
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(20)

xform = svtk.svtkTransformCoordinateSystems()
xform.SetInputConnection(sphere.GetOutputPort())
xform.SetInputCoordinateSystemToWorld()
xform.SetOutputCoordinateSystemToDisplay()
xform.SetViewport(ren1)

gs = svtk.svtkGlyphSource2D()
gs.SetGlyphTypeToCircle()
gs.SetScale(20)
gs.FilledOff()
gs.CrossOn()
gs.Update()

# Create a table of glyphs
glypher = svtk.svtkGlyph2D()
glypher.SetInputConnection(xform.GetOutputPort())
glypher.SetSourceData(0, gs.GetOutput())
glypher.SetScaleModeToDataScalingOff()

mapper = svtk.svtkPolyDataMapper2D()
mapper.SetInputConnection(glypher.GetOutputPort())

glyphActor = svtk.svtkActor2D()
glyphActor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(glyphActor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()

# render the image
#iren.Start()
