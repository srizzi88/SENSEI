#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Test alternative methods of probing data including
# using svtkFindCellStrategy and directly specifying
# a cell locator.

# Control test size
res = 50

# create planes
# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(SVTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(SVTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

# Probe with three separate planes. Use different probing approaches on each
# of the three planes (svtkDataSet::FindCell(), directly specifying a cell
# locator, and using a svtkFindCellStrategy. Then isocontour the planes.

# First plane
plane = svtk.svtkPlaneSource()
plane.SetResolution(res,res)
transP1 = svtk.svtkTransform()
transP1.Translate(3.7,0.0,28.37)
transP1.Scale(5,5,5)
transP1.RotateY(90)
tpd1 = svtk.svtkTransformPolyDataFilter()
tpd1.SetInputConnection(plane.GetOutputPort())
tpd1.SetTransform(transP1)
probe1 = svtk.svtkProbeFilter()
probe1.SetInputConnection(tpd1.GetOutputPort())
probe1.SetSourceData(output)
probe1.DebugOn()
contour1 = svtk.svtkContourFilter()
contour1.SetInputConnection(probe1.GetOutputPort())
contour1.GenerateValues(50,output.GetScalarRange())
mapProbe1 = svtk.svtkPolyDataMapper()
mapProbe1.SetInputConnection(contour1.GetOutputPort())
mapProbe1.SetScalarRange(output.GetScalarRange())
probe1Actor = svtk.svtkActor()
probe1Actor.SetMapper(mapProbe1)

outTpd1 = svtk.svtkOutlineFilter()
outTpd1.SetInputConnection(tpd1.GetOutputPort())
mapTpd1 = svtk.svtkPolyDataMapper()
mapTpd1.SetInputConnection(outTpd1.GetOutputPort())
tpd1Actor = svtk.svtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0,0,0)

# Next plane
transP2 = svtk.svtkTransform()
transP2.Translate(9.2,0.0,31.20)
transP2.Scale(5,5,5)
transP2.RotateY(90)
tpd2 = svtk.svtkTransformPolyDataFilter()
tpd2.SetInputConnection(plane.GetOutputPort())
tpd2.SetTransform(transP2)
cellLoc = svtk.svtkStaticCellLocator()
probe2 = svtk.svtkProbeFilter()
probe2.SetInputConnection(tpd2.GetOutputPort())
probe2.SetSourceData(output)
probe2.SetCellLocatorPrototype(cellLoc)
probe2.DebugOn()
contour2 = svtk.svtkContourFilter()
contour2.SetInputConnection(probe2.GetOutputPort())
contour2.GenerateValues(50,output.GetScalarRange())
mapProbe2 = svtk.svtkPolyDataMapper()
mapProbe2.SetInputConnection(contour2.GetOutputPort())
mapProbe2.SetScalarRange(output.GetScalarRange())
probe2Actor = svtk.svtkActor()
probe2Actor.SetMapper(mapProbe2)

outTpd2 = svtk.svtkOutlineFilter()
outTpd2.SetInputConnection(tpd2.GetOutputPort())
mapTpd2 = svtk.svtkPolyDataMapper()
mapTpd2.SetInputConnection(outTpd2.GetOutputPort())
tpd2Actor = svtk.svtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0,0,0)

# Third plane
transP3 = svtk.svtkTransform()
transP3.Translate(13.27,0.0,33.30)
transP3.Scale(5,5,5)
transP3.RotateY(90)
tpd3 = svtk.svtkTransformPolyDataFilter()
tpd3.SetInputConnection(plane.GetOutputPort())
tpd3.SetTransform(transP3)
strategy = svtk.svtkCellLocatorStrategy()
probe3 = svtk.svtkProbeFilter()
probe3.SetInputConnection(tpd3.GetOutputPort())
probe3.SetSourceData(output)
probe3.SetFindCellStrategy(strategy)
probe3.DebugOn()
contour3 = svtk.svtkContourFilter()
contour3.SetInputConnection(probe3.GetOutputPort())
contour3.GenerateValues(50,output.GetScalarRange())
mapProbe3 = svtk.svtkPolyDataMapper()
mapProbe3.SetInputConnection(contour3.GetOutputPort())
mapProbe3.SetScalarRange(output.GetScalarRange())
probe3Actor = svtk.svtkActor()
probe3Actor.SetMapper(mapProbe3)

outTpd3 = svtk.svtkOutlineFilter()
outTpd3.SetInputConnection(tpd3.GetOutputPort())
mapTpd3 = svtk.svtkPolyDataMapper()
mapTpd3.SetInputConnection(outTpd3.GetOutputPort())
tpd3Actor = svtk.svtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0,0,0)

# Create an outline around the structured grid
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Add the actors
ren1.AddActor(outlineActor)
ren1.AddActor(probe1Actor)
ren1.AddActor(probe2Actor)
ren1.AddActor(probe3Actor)
ren1.AddActor(tpd1Actor)
ren1.AddActor(tpd2Actor)
ren1.AddActor(tpd3Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)

# Setup a camera view
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)

iren.Initialize()
renWin.Render()
iren.Start()
# --- end of script --
