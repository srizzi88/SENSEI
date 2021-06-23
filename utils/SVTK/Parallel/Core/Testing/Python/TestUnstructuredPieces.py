#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


math = svtk.svtkMath()
math.RandomSeed(22)

pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

dst = svtk.svtkDataSetTriangleFilter()
dst.SetInputData(output)

extract = svtk.svtkExtractUnstructuredGridPiece()
extract.SetInputConnection(dst.GetOutputPort())

cf = svtk.svtkContourFilter()
cf.SetInputConnection(extract.GetOutputPort())
cf.SetValue(0, 0.24)

pdn = svtk.svtkPolyDataNormals()
pdn.SetInputConnection(cf.GetOutputPort())

ps = svtk.svtkPieceScalars()
ps.SetInputConnection(pdn.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(3)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren = svtk.svtkRenderer()
ren.AddActor(actor)
ren.ResetCamera()

camera = ren.GetActiveCamera()
# $camera SetPosition 68.1939 -23.4323 12.6465
# $camera SetViewUp 0.46563 0.882375 0.0678508
# $camera SetFocalPoint 3.65707 11.4552 1.83509
# $camera SetClippingRange 59.2626 101.825

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()

#iren.Start()
