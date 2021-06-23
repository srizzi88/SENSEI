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
reader = svtk.svtkEnSightMasterServerReader()
# Make sure all algorithms use the composite data pipeline
cdp = svtk.svtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/elements.sos")
reader.SetCurrentPiece(0)
geom0 = svtk.svtkGeometryFilter()
geom0.SetInputConnection(reader.GetOutputPort())
mapper0 = svtk.svtkHierarchicalPolyDataMapper()
mapper0.SetInputConnection(geom0.GetOutputPort())
mapper0.SetColorModeToMapScalars()
mapper0.SetScalarModeToUsePointFieldData()
mapper0.ColorByArrayComponent("pointScalars",0)
mapper0.SetScalarRange(0,112)
actor0 = svtk.svtkActor()
actor0.SetMapper(mapper0)
# assign our actor to the renderer
ren1.AddActor(actor0)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
