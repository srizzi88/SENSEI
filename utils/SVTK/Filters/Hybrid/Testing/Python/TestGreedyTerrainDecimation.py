#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

lut = svtk.svtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Read the data: a height field results
demReader = svtk.svtkDEMReader()
demReader.SetFileName(SVTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]

# Decimate the terrain
deci = svtk.svtkGreedyTerrainDecimation()
deci.SetInputConnection(demReader.GetOutputPort())
deci.BoundaryVertexDeletionOn()
#  deci.SetErrorMeasureToSpecifiedReduction()
#  deci.SetReduction(0.95)
deci.SetErrorMeasureToNumberOfTriangles()
deci.SetNumberOfTriangles(5000)
#  deci.SetErrorMeasureToAbsoluteError()
#  deci.SetAbsoluteError(25.0)
#  deci.SetErrorMeasureToRelativeError()
#  deci.SetAbsoluteError(0.01)

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(deci.GetOutputPort())
normals.SetFeatureAngle(60)
normals.ConsistencyOn()
normals.SplittingOff()

demMapper = svtk.svtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

actor = svtk.svtkLODActor()
actor.SetMapper(demMapper)

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

ren1.SetBackground(.1, .2, .4)
iren.SetDesiredUpdateRate(5)

ren1.GetActiveCamera().SetViewUp(0, 0, 1)
ren1.GetActiveCamera().SetPosition(-99900, -21354, 131801)
ren1.GetActiveCamera().SetFocalPoint(41461, 41461, 2815)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()

renWin.Render()

iren.Initialize()
#iren.Start()
