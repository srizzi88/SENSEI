#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# we need to use composite data pipeline with multiblock datasets
alg = svtk.svtkAlgorithm()
pip = svtk.svtkCompositeDataPipeline()
alg.SetDefaultExecutivePrototype(pip)
del pip
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = svtk.svtkEnSightGoldBinaryReader()
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/naca.bin.case")
reader.SetTimeValue(3)
lut = svtk.svtkLookupTable()
lut.SetHueRange(0.667,0.0)
lut.SetTableRange(0.636,1.34)
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
blockMapper0 = svtk.svtkHierarchicalPolyDataMapper()
blockMapper0.SetInputConnection(geom.GetOutputPort())
blockActor0 = svtk.svtkActor()
blockActor0.SetMapper(blockMapper0)
ren1.AddActor(blockActor0)
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.SetFocalPoint(0,0,0)
cam1.ParallelProjectionOff()
cam1.Zoom(70)
cam1.SetViewAngle(1.0)
renWin.SetSize(400,400)
iren.Initialize()
alg.SetDefaultExecutivePrototype(None)
# --- end of script --
