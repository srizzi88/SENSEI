#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui
# create a sphere source and actor
#
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(16)
sphere.SetRadius(1.5)

# Extraction stuff
t = svtk.svtkTransform()
t.RotateX(90)
cylfunc = svtk.svtkCylinder()
cylfunc.SetRadius(0.5)
cylfunc.SetTransform(t)
extract = svtk.svtkExtractPolyDataGeometry()
extract.SetInputConnection(sphere.GetOutputPort())
extract.SetImplicitFunction(cylfunc)
extract.ExtractBoundaryCellsOn()
extract.PassPointsOn()
sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(extract.GetOutputPort())
sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)

# Extraction stuff - now cull points
extract2 = svtk.svtkExtractPolyDataGeometry()
extract2.SetInputConnection(sphere.GetOutputPort())
extract2.SetImplicitFunction(cylfunc)
extract2.ExtractBoundaryCellsOn()
extract2.PassPointsOff()
sphereMapper2 = svtk.svtkPolyDataMapper()
sphereMapper2.SetInputConnection(extract2.GetOutputPort())
sphereActor2 = svtk.svtkActor ()
sphereActor2.SetMapper(sphereMapper2)
sphereActor2.AddPosition(2.5, 0, 0)

# Put some glyphs on the points
glyphSphere = svtk.svtkSphereSource()
glyphSphere.SetRadius(0.05)
glyph = svtk.svtkGlyph3D()
glyph.SetInputConnection(extract.GetOutputPort())
glyph.SetSourceConnection(glyphSphere.GetOutputPort())
glyph.SetScaleModeToDataScalingOff()
glyphMapper = svtk.svtkPolyDataMapper()
glyphMapper.SetInputConnection(glyph.GetOutputPort())
glyphActor = svtk.svtkActor()
glyphActor.SetMapper(glyphMapper)

glyph2 = svtk.svtkGlyph3D()
glyph2.SetInputConnection(extract2.GetOutputPort())
glyph2.SetSourceConnection(glyphSphere.GetOutputPort())
glyph2.SetScaleModeToDataScalingOff()
glyphMapper2 = svtk.svtkPolyDataMapper()
glyphMapper2.SetInputConnection(glyph2.GetOutputPort())
glyphActor2 = svtk.svtkActor()
glyphActor2.SetMapper(glyphMapper2)
glyphActor2.AddPosition(2.5, 0, 0)


# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetWindowName("svtk - extractPolyData")
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddActor(glyphActor)
ren1.AddActor(sphereActor2)
ren1.AddActor(glyphActor2)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
