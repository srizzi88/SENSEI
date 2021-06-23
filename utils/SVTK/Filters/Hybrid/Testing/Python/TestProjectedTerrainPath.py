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

surface = svtk.svtkImageDataGeometryFilter()
surface.SetInputConnection(demReader.GetOutputPort())

warp = svtk.svtkWarpScalar()
warp.SetInputConnection(surface.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(warp.GetOutputPort())
normals.SetFeatureAngle(60)
normals.SplittingOff()

demMapper = svtk.svtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

demActor = svtk.svtkLODActor()
demActor.SetMapper(demMapper)

# Create some paths
pts = svtk.svtkPoints()
pts.InsertNextPoint(562669, 5.1198e+006, 1992.77)
pts.InsertNextPoint(562801, 5.11618e+006, 2534.97)
pts.InsertNextPoint(562913, 5.11157e+006, 1911.1)
pts.InsertNextPoint(559849, 5.11083e+006, 1681.34)
pts.InsertNextPoint(562471, 5.11633e+006, 2593.57)
pts.InsertNextPoint(563223, 5.11616e+006, 2598.31)
pts.InsertNextPoint(566579, 5.11127e+006, 1697.83)
pts.InsertNextPoint(569000, 5.11127e+006, 1697.83)

lines = svtk.svtkCellArray()
lines.InsertNextCell(3)
lines.InsertCellPoint(0)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)
lines.InsertNextCell(5)
lines.InsertCellPoint(3)
lines.InsertCellPoint(4)
lines.InsertCellPoint(5)
lines.InsertCellPoint(6)
lines.InsertCellPoint(7)

terrainPaths = svtk.svtkPolyData()
terrainPaths.SetPoints(pts)
terrainPaths.SetLines(lines)

projectedPaths = svtk.svtkProjectedTerrainPath()
projectedPaths.SetInputData(terrainPaths)
projectedPaths.SetSourceConnection(demReader.GetOutputPort())
projectedPaths.SetHeightOffset(25)
projectedPaths.SetHeightTolerance(5)
projectedPaths.SetProjectionModeToNonOccluded()
projectedPaths.SetProjectionModeToHug()

pathMapper = svtk.svtkPolyDataMapper()
pathMapper.SetInputConnection(projectedPaths.GetOutputPort())

paths = svtk.svtkActor()
paths.SetMapper(pathMapper)
paths.GetProperty().SetColor(1, 0, 0)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(demActor)
ren1.AddActor(paths)
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
