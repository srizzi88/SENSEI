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
del cdp
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/elements6.case")
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
calc = svtk.svtkArrayCalculator()
calc.SetInputConnection(geom.GetOutputPort())
calc.SetAttributeTypeToPointData()
calc.SetFunction("pointCVectors_r . pointCVectors_i + pointScalars")
calc.AddScalarArrayName("pointScalars",0)
calc.AddVectorArrayName("pointCVectors_r",0,1,2)
calc.AddVectorArrayName("pointCVectors_i",0,1,2)
calc.SetResultArrayName("test")
mapper = svtk.svtkHierarchicalPolyDataMapper()
mapper.SetInputConnection(calc.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUsePointFieldData()
mapper.ColorByArrayComponent("test",0)
mapper.SetScalarRange(0,36000)
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# assign our actor to the renderer
ren1.AddActor(actor)
# enable user interface interactor
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
