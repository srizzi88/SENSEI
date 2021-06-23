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
# create pipeline
#
reader = svtk.svtkSTLReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/42400-IDGH.stl")
dicer = svtk.svtkOBBDicer()
dicer.SetInputConnection(reader.GetOutputPort())
dicer.SetNumberOfPointsPerPiece(1000)
dicer.Update()
isoMapper = svtk.svtkDataSetMapper()
isoMapper.SetInputConnection(dicer.GetOutputPort())
isoMapper.SetScalarRange(0,dicer.GetNumberOfActualPieces())
isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(0.7,0.3,0.3)
outline = svtk.svtkOutlineCornerFilter()
outline.SetInputConnection(reader.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
ren1.SetBackground(0.5,0.5,0.6)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
