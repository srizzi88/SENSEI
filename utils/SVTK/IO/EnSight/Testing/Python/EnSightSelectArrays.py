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
reader = svtk.svtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = svtk.svtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/blow1_ascii.case")
reader.SetTimeValue(1)
reader.ReadAllVariablesOff()
reader.SetPointArrayStatus("displacement",1)
reader.SetCellArrayStatus("thickness",1)
reader.SetCellArrayStatus("displacement",1)
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
mapper = svtk.svtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetScalarRange(0.5,1.0)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# assign our actor to the renderer
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
ren1.GetActiveCamera().SetPosition(99.3932,17.6571,-22.6071)
ren1.GetActiveCamera().SetFocalPoint(3.5,12,1.5)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0.239617,-0.01054,0.97081)
ren1.ResetCameraClippingRange()
renWin.Render()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
