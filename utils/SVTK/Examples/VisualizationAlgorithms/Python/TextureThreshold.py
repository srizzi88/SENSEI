#!/usr/bin/env python

# This example shows how to use a transparent texture map to perform
# thresholding. The key is the svtkThresholdTextureCoords filter which
# creates texture coordinates based on a threshold value. These
# texture coordinates are used in conjuntion with a texture map with
# varying opacity and intensity to create an inside, transition, and
# outside region.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Begin by reading some structure grid data.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/bluntfinxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/bluntfinq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# Now extract surfaces from the grid corresponding to boundary
# geometry.  First the wall.
wall = svtk.svtkStructuredGridGeometryFilter()
wall.SetInputData(pl3d_output)
wall.SetExtent(0, 100, 0, 0, 0, 100)
wallMap = svtk.svtkPolyDataMapper()
wallMap.SetInputConnection(wall.GetOutputPort())
wallMap.ScalarVisibilityOff()
wallActor = svtk.svtkActor()
wallActor.SetMapper(wallMap)
wallActor.GetProperty().SetColor(0.8, 0.8, 0.8)

# Now the fin.
fin = svtk.svtkStructuredGridGeometryFilter()
fin.SetInputData(pl3d_output)
fin.SetExtent(0, 100, 0, 100, 0, 0)
finMap = svtk.svtkPolyDataMapper()
finMap.SetInputConnection(fin.GetOutputPort())
finMap.ScalarVisibilityOff()
finActor = svtk.svtkActor()
finActor.SetMapper(finMap)
finActor.GetProperty().SetColor(0.8, 0.8, 0.8)

# Extract planes to threshold. Start by reading the specially designed
# texture map that has three regions: an inside, boundary, and outside
# region. The opacity and intensity of this texture map are varied.
tmap = svtk.svtkStructuredPointsReader()
tmap.SetFileName(SVTK_DATA_ROOT + "/Data/texThres2.svtk")
texture = svtk.svtkTexture()
texture.SetInputConnection(tmap.GetOutputPort())
texture.InterpolateOff()
texture.RepeatOff()

# Here are the three planes which will be texture thresholded.
plane1 = svtk.svtkStructuredGridGeometryFilter()
plane1.SetInputData(pl3d_output)
plane1.SetExtent(10, 10, 0, 100, 0, 100)
thresh1 = svtk.svtkThresholdTextureCoords()
thresh1.SetInputConnection(plane1.GetOutputPort())
thresh1.ThresholdByUpper(1.5)
plane1Map = svtk.svtkDataSetMapper()
plane1Map.SetInputConnection(thresh1.GetOutputPort())
plane1Map.SetScalarRange(pl3d_output.GetScalarRange())
plane1Actor = svtk.svtkActor()
plane1Actor.SetMapper(plane1Map)
plane1Actor.SetTexture(texture)
plane1Actor.GetProperty().SetOpacity(0.999)

plane2 = svtk.svtkStructuredGridGeometryFilter()
plane2.SetInputData(pl3d_output)
plane2.SetExtent(30, 30, 0, 100, 0, 100)
thresh2 = svtk.svtkThresholdTextureCoords()
thresh2.SetInputConnection(plane2.GetOutputPort())
thresh2.ThresholdByUpper(1.5)
plane2Map = svtk.svtkDataSetMapper()
plane2Map.SetInputConnection(thresh2.GetOutputPort())
plane2Map.SetScalarRange(pl3d_output.GetScalarRange())
plane2Actor = svtk.svtkActor()
plane2Actor.SetMapper(plane2Map)
plane2Actor.SetTexture(texture)
plane2Actor.GetProperty().SetOpacity(0.999)

plane3 = svtk.svtkStructuredGridGeometryFilter()
plane3.SetInputData(pl3d_output)
plane3.SetExtent(35, 35, 0, 100, 0, 100)
thresh3 = svtk.svtkThresholdTextureCoords()
thresh3.SetInputConnection(plane3.GetOutputPort())
thresh3.ThresholdByUpper(1.5)
plane3Map = svtk.svtkDataSetMapper()
plane3Map.SetInputConnection(thresh3.GetOutputPort())
plane3Map.SetScalarRange(pl3d_output.GetScalarRange())
plane3Actor = svtk.svtkActor()
plane3Actor.SetMapper(plane3Map)
plane3Actor.SetTexture(texture)
plane3Actor.GetProperty().SetOpacity(0.999)

# For context create an outline around the data.
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0, 0, 0)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(wallActor)
ren.AddActor(finActor)
ren.AddActor(plane1Actor)
ren.AddActor(plane2Actor)
ren.AddActor(plane3Actor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)

# Set up a nice view.
cam1 = svtk.svtkCamera()
cam1.SetClippingRange(1.51176, 75.5879)
cam1.SetFocalPoint(2.33749, 2.96739, 3.61023)
cam1.SetPosition(10.8787, 5.27346, 15.8687)
cam1.SetViewAngle(30)
cam1.SetViewUp(-0.0610856, 0.987798, -0.143262)
ren.SetActiveCamera(cam1)

iren.Initialize()
renWin.Render()
iren.Start()
