#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# create tensor ellipsoids
# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# Create tensor ellipsoids
#
# generate tensors
ptLoad = svtk.svtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(6,6,6)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)
# extract plane of data
plane = svtk.svtkImageDataGeometryFilter()
plane.SetInputConnection(ptLoad.GetOutputPort())
plane.SetExtent(2,2,0,99,0,99)
# Generate ellipsoids
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(8)
ellipsoids = svtk.svtkTensorGlyph()
ellipsoids.SetInputConnection(ptLoad.GetOutputPort())
ellipsoids.SetSourceConnection(sphere.GetOutputPort())
ellipsoids.SetScaleFactor(10)
ellipsoids.ClampScalingOn()
ellipNormals = svtk.svtkPolyDataNormals()
ellipNormals.SetInputConnection(ellipsoids.GetOutputPort())
# Map contour
lut = svtk.svtkLogLookupTable()
lut.SetHueRange(.6667,0.0)
ellipMapper = svtk.svtkPolyDataMapper()
ellipMapper.SetInputConnection(ellipNormals.GetOutputPort())
ellipMapper.SetLookupTable(lut)
plane.Update()
#force update for scalar range
ellipMapper.SetScalarRange(plane.GetOutput().GetScalarRange())
ellipActor = svtk.svtkActor()
ellipActor.SetMapper(ellipMapper)
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
ren1.AddActor(ellipActor)
ren1.AddActor(outlineActor)
ren1.AddActor(coneActor)
ren1.SetBackground(1.0,1.0,1.0)
ren1.SetActiveCamera(camera)
renWin.SetSize(400,400)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
