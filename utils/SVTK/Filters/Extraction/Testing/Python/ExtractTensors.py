#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create tensor ellipsoids
# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ptLoad = svtk.svtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(30,30,30)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)
extractTensor = svtk.svtkExtractTensorComponents()
extractTensor.SetInputConnection(ptLoad.GetOutputPort())
extractTensor.ScalarIsEffectiveStress()
extractTensor.ScalarIsComponent()
extractTensor.ExtractScalarsOn()
extractTensor.ExtractVectorsOn()
extractTensor.ExtractNormalsOff()
extractTensor.ExtractTCoordsOn()
contour = svtk.svtkContourFilter()
contour.SetInputConnection(extractTensor.GetOutputPort())
contour.SetValue(0,0)
probe = svtk.svtkProbeFilter()
probe.SetInputConnection(contour.GetOutputPort())
probe.SetSourceConnection(ptLoad.GetOutputPort())
su = svtk.svtkLoopSubdivisionFilter()
su.SetInputConnection(probe.GetOutputPort())
su.SetNumberOfSubdivisions(1)
s1Mapper = svtk.svtkPolyDataMapper()
s1Mapper.SetInputConnection(probe.GetOutputPort())
#    s1Mapper SetInputConnection [su GetOutputPort]
s1Actor = svtk.svtkActor()
s1Actor.SetMapper(s1Mapper)
#
# plane for context
#
g = svtk.svtkImageDataGeometryFilter()
g.SetInputConnection(ptLoad.GetOutputPort())
g.SetExtent(0,100,0,100,0,0)
g.Update()
#for scalar range
gm = svtk.svtkPolyDataMapper()
gm.SetInputConnection(g.GetOutputPort())
gm.SetScalarRange(g.GetOutput().GetScalarRange())
ga = svtk.svtkActor()
ga.SetMapper(gm)
s1Mapper.SetScalarRange(g.GetOutput().GetScalarRange())
#
# Create outline around data
#
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(ptLoad.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
#
# Create cone indicating application of load
#
coneSrc = svtk.svtkConeSource()
coneSrc.SetRadius(.5)
coneSrc.SetHeight(2)
coneMap = svtk.svtkPolyDataMapper()
coneMap.SetInputConnection(coneSrc.GetOutputPort())
coneActor = svtk.svtkActor()
coneActor.SetMapper(coneMap)
coneActor.SetPosition(0,0,11)
coneActor.RotateY(90)
coneActor.GetProperty().SetColor(1,0,0)
camera = svtk.svtkCamera()
camera.SetFocalPoint(0.113766,-1.13665,-1.01919)
camera.SetPosition(-29.4886,-63.1488,26.5807)
camera.SetViewAngle(24.4617)
camera.SetViewUp(0.17138,0.331163,0.927879)
camera.SetClippingRange(1,100)
ren1.AddActor(s1Actor)
ren1.AddActor(outlineActor)
ren1.AddActor(coneActor)
ren1.AddActor(ga)
ren1.SetBackground(1.0,1.0,1.0)
ren1.SetActiveCamera(camera)
renWin.SetSize(300,300)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
