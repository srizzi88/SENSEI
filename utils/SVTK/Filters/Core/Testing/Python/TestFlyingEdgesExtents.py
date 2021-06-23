#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
ren2 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
ren1.SetViewport(0,0,0.5,1)
ren2.SetViewport(0.5,0,1,1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Test the negative extents
source = svtk.svtkRTAnalyticSource()

iso = svtk.svtkFlyingEdges3D()
iso.SetInputConnection(source.GetOutputPort())
iso.SetValue(0,150)

isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(1)

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(source.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Test the non-x-edge-intersecting contours and cuts
res = 100
plane = svtk.svtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(0,1,0)

sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(plane)
sample.SetModelBounds(-10,10, -10,10, -10,10)
sample.SetSampleDimensions(res,res,res)
sample.SetOutputScalarTypeToFloat()
sample.Update()

iso2 = svtk.svtkFlyingEdges3D()
iso2.SetInputConnection(sample.GetOutputPort())
iso2.SetValue(0,0.0)

isoMapper2 = svtk.svtkPolyDataMapper()
isoMapper2.SetInputConnection(iso2.GetOutputPort())
isoMapper2.ScalarVisibilityOff()

isoActor2 = svtk.svtkActor()
isoActor2.SetMapper(isoMapper2)
isoActor2.GetProperty().SetColor(1,1,1)
isoActor2.GetProperty().SetOpacity(1)

outline2 = svtk.svtkOutlineFilter()
outline2.SetInputConnection(sample.GetOutputPort())

outlineMapper2 = svtk.svtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())

outlineActor2 = svtk.svtkActor()
outlineActor2.SetMapper(outlineMapper)

# Extract a slice in the desired orientation
center = [0.0, 0.0, 0.0]
axial = svtk.svtkMatrix4x4()
axial.DeepCopy((1, 0, 0, center[0],
                0, 1, 0, center[1],
                0, 0, 1, center[2],
                0, 0, 0, 1))

reslice = svtk.svtkImageReslice()
reslice.SetInputConnection(sample.GetOutputPort())
reslice.SetOutputDimensionality(2)
reslice.SetResliceAxes(axial)
reslice.SetInterpolationModeToLinear()

iso3 = svtk.svtkFlyingEdges2D()
iso3.SetInputConnection(reslice.GetOutputPort())
iso3.SetValue(0,0.0)

tube = svtk.svtkTubeFilter()
tube.SetInputConnection(iso3.GetOutputPort())
tube.SetRadius(0.25)

mapper3 = svtk.svtkPolyDataMapper()
mapper3.SetInputConnection(tube.GetOutputPort())

actor3 = svtk.svtkActor()
actor3.SetMapper(mapper3)


# Add the actors to the renderer, set the background and size
#
renWin.SetSize(600,300)
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
ren1.ResetCamera()

cam = svtk.svtkCamera()
cam.SetPosition(0,1,0)
cam.SetFocalPoint(0,0,0)
cam.SetViewUp(0,0,1)
ren2.SetActiveCamera(cam)
ren2.AddActor(outlineActor2)
ren2.AddActor(isoActor2)
ren2.AddActor(actor3)
ren2.SetBackground(0,0,0)
ren2.ResetCamera()

iren.Initialize()

renWin.Render()
# --- end of script --
