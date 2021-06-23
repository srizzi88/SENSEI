#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
aPlane = svtk.svtkPlaneSource()
aPlane.SetCenter(-100,-100,-100)
aPlane.SetOrigin(-100,-100,-100)
aPlane.SetPoint1(-90,-100,-100)
aPlane.SetPoint2(-100,-90,-100)
aPlane.SetNormal(0,-1,1)
imageIn = svtk.svtkPNMReader()
imageIn.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/earth.ppm")
texture = svtk.svtkTexture()
texture.SetInputConnection(imageIn.GetOutputPort())
texturePlane = svtk.svtkTextureMapToPlane()
texturePlane.SetInputConnection(aPlane.GetOutputPort())
texturePlane.AutomaticPlaneGenerationOn()
planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(texturePlane.GetOutputPort())
texturedPlane = svtk.svtkActor()
texturedPlane.SetMapper(planeMapper)
texturedPlane.SetTexture(texture)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(texturedPlane)
#ren1 SetBackground 1 1 1
renWin.SetSize(200,200)
renWin.Render()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
