#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read some Fluent GAMBIT in ASCII form
reader = svtk.svtkGAMBITReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/prism.neu")
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(reader.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
renWin.SetSize(300,300)
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
