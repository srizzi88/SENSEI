#!/usr/bin/env python
# import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create pipeline - rectilinear grid
#
rgridReader = svtk.svtkRectilinearGridReader()
rgridReader.SetFileName(SVTK_DATA_ROOT + "/Data/RectGrid2.svtk")

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(rgridReader.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(outline.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

rgridReader.Update()

extract1 = svtk.svtkExtractRectilinearGrid()
extract1.SetInputConnection(rgridReader.GetOutputPort())
# extract1.SetVOI(0, 46, 0, 32, 0, 10)
extract1.SetVOI(23, 40, 16, 30, 9, 9)
extract1.SetSampleRate(2, 2, 1)
extract1.IncludeBoundaryOn()
extract1.Update()

surf1 = svtk.svtkDataSetSurfaceFilter()
surf1.SetInputConnection(extract1.GetOutputPort())

tris = svtk.svtkTriangleFilter()
tris.SetInputConnection(surf1.GetOutputPort())

mapper1 = svtk.svtkPolyDataMapper()
mapper1.SetInputConnection(tris.GetOutputPort())
mapper1.SetScalarRange(extract1.GetOutput().GetScalarRange())

actor1 = svtk.svtkActor()
actor1.SetMapper(mapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# ren1.AddActor(actor)
ren1.AddActor(actor1)
renWin.SetSize(340, 400)

iren.Initialize()

# render the image
#iren.Start()
