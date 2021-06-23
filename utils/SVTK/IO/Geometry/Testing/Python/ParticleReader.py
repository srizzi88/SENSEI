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
reader = svtk.svtkParticleReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/Particles.raw")
reader.SetDataByteOrderToBigEndian()
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(reader.GetOutputPort())
mapper.SetScalarRange(4,9)
mapper.SetPiece(1)
mapper.SetNumberOfPieces(2)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetPointSize(2.5)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)
# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
