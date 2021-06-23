#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some AVS UCD data in ASCII form
r = svtk.svtkAVSucdReader()
r.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/cellsnd.ascii.inp")
AVSMapper = svtk.svtkDataSetMapper()
AVSMapper.SetInputConnection(r.GetOutputPort())
AVSActor = svtk.svtkActor()
AVSActor.SetMapper(AVSMapper)
# Read some AVS UCD data in binary form
r2 = svtk.svtkAVSucdReader()
r2.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/cellsnd.bin.inp")
AVSMapper2 = svtk.svtkDataSetMapper()
AVSMapper2.SetInputConnection(r2.GetOutputPort())
AVSActor2 = svtk.svtkActor()
AVSActor2.SetMapper(AVSMapper2)
AVSActor2.AddPosition(5,0,0)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(AVSActor)
ren1.AddActor(AVSActor2)
renWin.SetSize(300,150)
iren.Initialize()
renWin.Render()
ren1.GetActiveCamera().Zoom(2)
# prevent the tk window from showing up then start the event loop
# --- end of script --
