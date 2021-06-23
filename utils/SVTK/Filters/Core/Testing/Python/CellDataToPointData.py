#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# cell scalars to point scalars
# get the interactor ui
# Create the RenderWindow, Renderer and RenderWindowInteractor
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create a 2*2 cell/3*3 pt structuredgrid
points = svtk.svtkPoints()
points.InsertNextPoint(-1,1,0)
points.InsertNextPoint(0,1,0)
points.InsertNextPoint(1,1,0)
points.InsertNextPoint(-1,0,0)
points.InsertNextPoint(0,0,0)
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(-1,-1,0)
points.InsertNextPoint(0,-1,0)
points.InsertNextPoint(1,-1,0)
faceColors = svtk.svtkFloatArray()
faceColors.InsertNextValue(0)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(2)
sgrid = svtk.svtkStructuredGrid()
sgrid.SetDimensions(3,3,1)
sgrid.SetPoints(points)
sgrid.GetCellData().SetScalars(faceColors)
Cell2Point = svtk.svtkCellDataToPointData()
Cell2Point.SetInputData(sgrid)
Cell2Point.PassCellDataOn()
Cell2Point.Update()
mapper = svtk.svtkDataSetMapper()
mapper.SetInputData(Cell2Point.GetStructuredGridOutput())
mapper.SetScalarModeToUsePointData()
mapper.SetScalarRange(0,2)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(256,256)
# render the image
iren.Initialize()
# --- end of script --
