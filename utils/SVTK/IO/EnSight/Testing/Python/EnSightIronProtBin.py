#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create a rendering window and renderer
ren1 = svtk.svtkRenderer()
ren1.SetBackground(0,0,0)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300,300)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# camera parameters
camera = ren1.GetActiveCamera()
camera.SetPosition(-54.8012,109.471,231.412)
camera.SetFocalPoint(33,33,33)
camera.SetViewUp(0.157687,0.942832,-0.293604)
camera.SetViewAngle(30)
camera.SetClippingRange(124.221,363.827)
reader = svtk.svtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = svtk.svtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/ironProt_bin.case")
Contour0 = svtk.svtkContourFilter()
Contour0.SetInputConnection(reader.GetOutputPort())
Contour0.SetValue(0,200)
Contour0.SetComputeScalars(1)
mapper = svtk.svtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(Contour0.GetOutputPort())
mapper.SetScalarRange(0,1)
mapper.SetScalarVisibility(1)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()
actor.GetProperty().SetInterpolationToGouraud()
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
