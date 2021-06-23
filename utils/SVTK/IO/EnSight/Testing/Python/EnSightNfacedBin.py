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
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/TEST_bin.case")
dss = svtk.svtkDataSetSurfaceFilter()
dss.SetInputConnection(reader.GetOutputPort())
mapper = svtk.svtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(dss.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.ColorByArrayComponent("Pressure",0)
mapper.SetScalarRange(0.121168,0.254608)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# assign our actor to the renderer
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
ren1.GetActiveCamera().SetPosition(0.643568,0.424804,-0.477458)
ren1.GetActiveCamera().SetFocalPoint(0.894177,0.490735,0.028153)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0.338885,0.896657,-0.284892)
ren1.ResetCameraClippingRange()
renWin.Render()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
