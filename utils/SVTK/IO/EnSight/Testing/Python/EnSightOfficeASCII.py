#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
reader = svtk.svtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = svtk.svtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(SVTK_DATA_ROOT) + "/Data/EnSight/office_ascii.case")
reader.Update()
outline = svtk.svtkStructuredGridOutlineFilter()
#    outline SetInputConnection [reader GetOutputPort]
outline.SetInputData(reader.GetOutput().GetBlock(0))
mapOutline = svtk.svtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
# Create source for streamtubes
streamer = svtk.svtkStreamTracer()
#    streamer SetInputConnection [reader GetOutputPort]
streamer.SetInputData(reader.GetOutput().GetBlock(0))
streamer.SetStartPosition(0.1,2.1,0.5)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.1)
streamer.SetIntegrationDirectionToForward()
cone = svtk.svtkConeSource()
cone.SetResolution(8)
cones = svtk.svtkGlyph3D()
cones.SetInputConnection(streamer.GetOutputPort())
cones.SetSourceConnection(cone.GetOutputPort())
cones.SetScaleFactor(3)
cones.SetInputArrayToProcess(1, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, "vectors")
cones.SetScaleModeToScaleByVector()
mapCones = svtk.svtkPolyDataMapper()
mapCones.SetInputConnection(cones.GetOutputPort())
#    eval mapCones SetScalarRange [[reader GetOutput] GetScalarRange]
mapCones.SetScalarRange(reader.GetOutput().GetBlock(0).GetScalarRange())
conesActor = svtk.svtkActor()
conesActor.SetMapper(mapCones)
ren1.AddActor(outlineActor)
ren1.AddActor(conesActor)
ren1.SetBackground(0.4,0.4,0.5)
renWin.SetSize(300,300)
iren.Initialize()
# interact with data
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
