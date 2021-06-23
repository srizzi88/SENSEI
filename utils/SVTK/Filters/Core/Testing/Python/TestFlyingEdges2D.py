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

# Pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")

iso = svtk.svtkFlyingEdges2D()
iso.SetInputConnection(reader.GetOutputPort())
iso.GenerateValues(12,500,1150)

isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
