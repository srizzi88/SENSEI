#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# read the football dataset:
#
reader = svtk.svtkUnstructuredGridReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/PentaHexa.svtk")
reader.Update()
# Clip
#
plane = svtk.svtkPlane()
plane.SetNormal(1,1,0)
clip = svtk.svtkClipDataSet()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetClipFunction(plane)
clip.GenerateClipScalarsOn()
g = svtk.svtkDataSetSurfaceFilter()
g.SetInputConnection(clip.GetOutputPort())
map = svtk.svtkPolyDataMapper()
map.SetInputConnection(g.GetOutputPort())
clipActor = svtk.svtkActor()
clipActor.SetMapper(map)
# Contour
#
contour = svtk.svtkContourFilter()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetValue(0,0.125)
contour.SetValue(1,0.25)
contour.SetValue(2,0.5)
contour.SetValue(3,0.75)
contour.SetValue(4,1.0)
g2 = svtk.svtkDataSetSurfaceFilter()
g2.SetInputConnection(contour.GetOutputPort())
map2 = svtk.svtkPolyDataMapper()
map2.SetInputConnection(g2.GetOutputPort())
map2.ScalarVisibilityOff()
contourActor = svtk.svtkActor()
contourActor.SetMapper(map2)
contourActor.GetProperty().SetColor(1,0,0)
contourActor.GetProperty().SetRepresentationToWireframe()
# Triangulate
tris = svtk.svtkDataSetTriangleFilter()
tris.SetInputConnection(reader.GetOutputPort())
shrink = svtk.svtkShrinkFilter()
shrink.SetInputConnection(tris.GetOutputPort())
shrink.SetShrinkFactor(.8)
map3 = svtk.svtkDataSetMapper()
map3.SetInputConnection(shrink.GetOutputPort())
map3.SetScalarRange(0,26)
triActor = svtk.svtkActor()
triActor.SetMapper(map3)
triActor.AddPosition(2,0,0)
# Create graphics stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(contourActor)
ren1.AddActor(triActor)
ren1.SetBackground(1,1,1)
renWin.Render()
# render the image
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
