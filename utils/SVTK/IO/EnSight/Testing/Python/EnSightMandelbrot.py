#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create a rendering window and renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader1 = svtk.svtkEnSightMasterServerReader()
# Make sure all algorithms use the composite data pipeline
cdp = svtk.svtkCompositeDataPipeline()
reader1.SetDefaultExecutivePrototype(cdp)
reader1.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/mandelbrot.sos")
reader1.SetCurrentPiece(0)
geom0 = svtk.svtkGeometryFilter()
geom0.SetInputConnection(reader1.GetOutputPort())
mapper0 = svtk.svtkHierarchicalPolyDataMapper()
mapper0.SetInputConnection(geom0.GetOutputPort())
mapper0.SetColorModeToMapScalars()
mapper0.SetScalarModeToUsePointFieldData()
mapper0.ColorByArrayComponent("Iterations",0)
mapper0.SetScalarRange(0,112)
actor0 = svtk.svtkActor()
actor0.SetMapper(mapper0)
reader2 = svtk.svtkEnSightMasterServerReader()
reader2.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/mandelbrot.sos")
reader2.SetCurrentPiece(1)
geom2 = svtk.svtkGeometryFilter()
geom2.SetInputConnection(reader2.GetOutputPort())
mapper2 = svtk.svtkHierarchicalPolyDataMapper()
mapper2.SetInputConnection(geom2.GetOutputPort())
mapper2.SetColorModeToMapScalars()
mapper2.SetScalarModeToUsePointFieldData()
mapper2.ColorByArrayComponent("Iterations",0)
mapper2.SetScalarRange(0,112)
actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
# assign our actor to the renderer
ren1.AddActor(actor0)
ren1.AddActor(actor2)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader1.SetDefaultExecutivePrototype(None)
# --- end of script --
