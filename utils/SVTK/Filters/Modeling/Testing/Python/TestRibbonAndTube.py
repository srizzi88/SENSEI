#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline
#
reader = svtk.svtkPolyDataReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/svtk.svtk")
# Read a ruler texture
r = svtk.svtkPNGReader()
r.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/ruler.png")
atext = svtk.svtkTexture()
atext.SetInputConnection(r.GetOutputPort())
atext.InterpolateOn()
# produce some ribbons
ribbon = svtk.svtkRibbonFilter()
ribbon.SetInputConnection(reader.GetOutputPort())
ribbon.SetWidth(0.1)
ribbon.SetGenerateTCoordsToUseLength()
ribbon.SetTextureLength(1.0)
ribbon.UseDefaultNormalOn()
ribbon.SetDefaultNormal(0,0,1)
ribbonMapper = svtk.svtkPolyDataMapper()
ribbonMapper.SetInputConnection(ribbon.GetOutputPort())
ribbonActor = svtk.svtkActor()
ribbonActor.SetMapper(ribbonMapper)
ribbonActor.GetProperty().SetColor(1,1,0)
ribbonActor.SetTexture(atext)
# produce some tubes
tuber = svtk.svtkTubeFilter()
tuber.SetInputConnection(reader.GetOutputPort())
tuber.SetRadius(0.1)
tuber.SetNumberOfSides(12)
tuber.SetGenerateTCoordsToUseLength()
tuber.SetTextureLength(0.5)
tuber.CappingOn()
tubeMapper = svtk.svtkPolyDataMapper()
tubeMapper.SetInputConnection(tuber.GetOutputPort())
tubeActor = svtk.svtkActor()
tubeActor.SetMapper(tubeMapper)
tubeActor.GetProperty().SetColor(1,1,0)
tubeActor.SetTexture(atext)
tubeActor.AddPosition(5,0,0)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(ribbonActor)
ren1.AddActor(tubeActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(900,350)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(4)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
threshold = 15
# --- end of script --
