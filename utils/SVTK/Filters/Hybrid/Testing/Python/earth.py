#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
tss = svtk.svtkTexturedSphereSource()
tss.SetThetaResolution(18)
tss.SetPhiResolution(9)
earthMapper = svtk.svtkPolyDataMapper()
earthMapper.SetInputConnection(tss.GetOutputPort())
earthActor = svtk.svtkActor()
earthActor.SetMapper(earthMapper)
# load in the texture map
#
atext = svtk.svtkTexture()
pnmReader = svtk.svtkPNMReader()
pnmReader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/earth.ppm")
atext.SetInputConnection(pnmReader.GetOutputPort())
atext.InterpolateOn()
earthActor.SetTexture(atext)
# create a earth source and actor
#
es = svtk.svtkEarthSource()
es.SetRadius(0.501)
es.SetOnRatio(2)
earth2Mapper = svtk.svtkPolyDataMapper()
earth2Mapper.SetInputConnection(es.GetOutputPort())
earth2Actor = svtk.svtkActor()
earth2Actor.SetMapper(earth2Mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(earthActor)
ren1.AddActor(earth2Actor)
ren1.SetBackground(0,0,0.1)
renWin.SetSize(300,300)
# render the image
#
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
