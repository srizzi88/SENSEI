#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

math = svtk.svtkMath()
math.RandomSeed(22)

sphere = svtk.svtkSphereSource()
sphere.SetPhiResolution(32)
sphere.SetThetaResolution(32)

extract = svtk.svtkExtractPolyDataPiece()
extract.SetInputConnection(sphere.GetOutputPort())

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(extract.GetOutputPort())

ps = svtk.svtkPieceScalars()
ps.SetInputConnection(normals.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(2)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

sphere2 = svtk.svtkSphereSource()
sphere2.SetPhiResolution(32)
sphere2.SetThetaResolution(32)

extract2 = svtk.svtkExtractPolyDataPiece()
extract2.SetInputConnection(sphere2.GetOutputPort())

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(extract2.GetOutputPort())
mapper2.SetNumberOfPieces(2)
mapper2.SetPiece(1)
mapper2.SetScalarRange(0, 4)
mapper2.SetScalarModeToUseCellFieldData()
mapper2.SetColorModeToMapScalars()
mapper2.ColorByArrayComponent(svtk.svtkDataSetAttributes.GhostArrayName(), 0)
mapper2.SetGhostLevel(4)

# check the pipeline size
extract2.UpdateInformation()
psize = svtk.svtkPipelineSize()
if (psize.GetEstimatedSize(extract2, 0, 0) > 100):
    print ("ERROR: Pipeline Size increased")
    pass
if (psize.GetNumberOfSubPieces(10, mapper2, mapper2.GetPiece(), mapper2.GetNumberOfPieces()) != 1):
    print ("ERROR: Number of sub pieces changed",
           psize.GetNumberOfSubPieces(10, mapper2, mapper2.GetPiece(), mapper2.GetNumberOfPieces()))
    pass

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5, 0, 0)

sphere3 = svtk.svtkSphereSource()
sphere3.SetPhiResolution(32)
sphere3.SetThetaResolution(32)

extract3 = svtk.svtkExtractPolyDataPiece()
extract3.SetInputConnection(sphere3.GetOutputPort())

ps3 = svtk.svtkPieceScalars()
ps3.SetInputConnection(extract3.GetOutputPort())

mapper3 = svtk.svtkPolyDataMapper()
mapper3.SetInputConnection(ps3.GetOutputPort())
mapper3.SetNumberOfSubPieces(8)
mapper3.SetScalarRange(0, 8)

actor3 = svtk.svtkActor()
actor3.SetMapper(mapper3)
actor3.SetPosition(0, -1.5, 0)

sphere4 = svtk.svtkSphereSource()
sphere4.SetPhiResolution(32)
sphere4.SetThetaResolution(32)

extract4 = svtk.svtkExtractPolyDataPiece()
extract4.SetInputConnection(sphere4.GetOutputPort())

ps4 = svtk.svtkPieceScalars()
ps4.RandomModeOn()
ps4.SetScalarModeToCellData()
ps4.SetInputConnection(extract4.GetOutputPort())

mapper4 = svtk.svtkPolyDataMapper()
mapper4.SetInputConnection(ps4.GetOutputPort())
mapper4.SetNumberOfSubPieces(8)
mapper4.SetScalarRange(0, 8)

actor4 = svtk.svtkActor()
actor4.SetMapper(mapper4)
actor4.SetPosition(1.5, -1.5, 0)

ren = svtk.svtkRenderer()
ren.AddActor(actor)
ren.AddActor(actor2)
ren.AddActor(actor3)
ren.AddActor(actor4)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
#iren.Start()
