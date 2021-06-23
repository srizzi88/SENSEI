#!/usr/bin/env python

# This tests svtkAMRSliceFilter

import svtk
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

def NumCells(out):
  n =0
  for i in range(out.GetNumberOfLevels()):
    for j in range(out.GetNumberOfDataSets(i)):
      m = out.GetDataSet(i,j).GetNumberOfCells()
      #print (i,j,m)
      n = n + m
  return n

filename= SVTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"
datafieldname = "TotalEnergy"

reader = svtk.svtkAMREnzoReader()
reader.SetFileName(filename)
reader.SetMaxLevel(10)
reader.SetCellArrayStatus(datafieldname,1)

filter = svtk.svtkAMRSliceFilter()
filter.SetInputConnection(reader.GetOutputPort())
filter.SetNormal(1)
filter.SetOffsetFromOrigin(0.86)
filter.SetMaxResolution(7)
filter.Update()
out = filter.GetOutputDataObject(0)
out.Audit()
if NumCells(out) != 800:
  print("Wrong number of cells in the output")
  exit(1)

# render
surface = svtk.svtkGeometryFilter()
surface.SetInputData(out)

mapper = svtk.svtkCompositePolyDataMapper2()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray(datafieldname)
mapper.SetScalarRange(1.2e-7, 1.5e-3)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

renderer = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(renderer)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renderer.AddActor(actor)
renderer.GetActiveCamera().SetPosition(1, 0, 0)
renderer.ResetCamera()
renWin.SetSize(300, 300)
iren.Initialize()
renWin.Render()
iren.Start()
