#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# this demonstrates appending data to generate an implicit model
lineX = svtk.svtkLineSource()
lineX.SetPoint1(-2.0,0.0,0.0)
lineX.SetPoint2(2.0,0.0,0.0)
lineX.Update()
lineY = svtk.svtkLineSource()
lineY.SetPoint1(0.0,-2.0,0.0)
lineY.SetPoint2(0.0,2.0,0.0)
lineY.Update()
lineZ = svtk.svtkLineSource()
lineZ.SetPoint1(0.0,0.0,-2.0)
lineZ.SetPoint2(0.0,0.0,2.0)
lineZ.Update()
aPlane = svtk.svtkPlaneSource()
aPlane.Update()
# set Data(0) "lineX"
# set Data(1) "lineY"
# set Data(2) "lineZ"
# set Data(3) "aPlane"
imp = svtk.svtkImplicitModeller()
imp.SetModelBounds(-2.5,2.5,-2.5,2.5,-2.5,2.5)
imp.SetSampleDimensions(60,60,60)
imp.SetCapValue(1000)
imp.SetProcessModeToPerVoxel()
# Okay now let's see if we can append
imp.StartAppend()
# for {set i 0} {$i < 4} {incr i} {
#     imp Append [$Data($i) GetOutput]
# }
imp.Append(lineX.GetOutput())
imp.Append(lineY.GetOutput())
imp.Append(lineZ.GetOutput())
imp.Append(aPlane.GetOutput())
imp.EndAppend()
cf = svtk.svtkContourFilter()
cf.SetInputConnection(imp.GetOutputPort())
cf.SetValue(0,0.1)
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(cf.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(imp.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
plane = svtk.svtkImageDataGeometryFilter()
plane.SetInputConnection(imp.GetOutputPort())
plane.SetExtent(0,60,0,60,30,30)
planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(plane.GetOutputPort())
planeMapper.SetScalarRange(0.197813,0.710419)
planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)
# graphics stuff
ren1 = svtk.svtkRenderer()
ren1.AddActor(actor)
ren1.AddActor(planeActor)
ren1.AddActor(outlineActor)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(0.1,0.2,0.4)
renWin.Render()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()
renWin.Render()
# --- end of script --
