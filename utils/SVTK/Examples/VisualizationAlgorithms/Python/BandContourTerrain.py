#!/usr/bin/env python

# In this example we show the use of the
# svtkBandedPolyDataContourFilter.  This filter creates separate,
# constant colored bands for a range of scalar values. Each band is
# bounded by two scalar values, and the cell data laying within the
# value has the same cell scalar value.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


# The lookup table is similar to that used by maps. Two hues are used:
# a brown for land, and a blue for water. The value of the hue is
# changed to give the effect of elevation.
Scale = 5
lutWater = svtk.svtkLookupTable()
lutWater.SetNumberOfColors(10)
lutWater.SetHueRange(0.58, 0.58)
lutWater.SetSaturationRange(0.5, 0.1)
lutWater.SetValueRange(0.5, 1.0)
lutWater.Build()
lutLand = svtk.svtkLookupTable()
lutLand.SetNumberOfColors(10)
lutLand.SetHueRange(0.1, 0.1)
lutLand.SetSaturationRange(0.4, 0.1)
lutLand.SetValueRange(0.55, 0.9)
lutLand.Build()


# The DEM reader reads data and creates an output image.
demModel = svtk.svtkDEMReader()
demModel.SetFileName(SVTK_DATA_ROOT + "/Data/SainteHelens.dem")
demModel.Update()

# We shrink the terrain data down a bit to yield better performance for
# this example.
shrinkFactor = 4
shrink = svtk.svtkImageShrink3D()
shrink.SetShrinkFactors(shrinkFactor, shrinkFactor, 1)
shrink.SetInputConnection(demModel.GetOutputPort())
shrink.AveragingOn()

# Convert the image into polygons.
geom = svtk.svtkImageDataGeometryFilter()
geom.SetInputConnection(shrink.GetOutputPort())

# Warp the polygons based on elevation.
warp = svtk.svtkWarpScalar()
warp.SetInputConnection(geom.GetOutputPort())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(Scale)

# Create the contour bands.
bcf = svtk.svtkBandedPolyDataContourFilter()
bcf.SetInputConnection(warp.GetOutputPort())
bcf.GenerateValues(15, demModel.GetOutput().GetScalarRange())
bcf.SetScalarModeToIndex()
bcf.GenerateContourEdgesOn()

# Compute normals to give a better look.
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(bcf.GetOutputPort())
normals.SetFeatureAngle(60)
normals.ConsistencyOff()
normals.SplittingOff()

demMapper = svtk.svtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(0, 10)
demMapper.SetLookupTable(lutLand)
demMapper.SetScalarModeToUseCellData()

demActor = svtk.svtkLODActor()
demActor.SetMapper(demMapper)

## Create contour edges
edgeMapper = svtk.svtkPolyDataMapper()
edgeMapper.SetInputConnection(bcf.GetOutputPort())
edgeMapper.SetResolveCoincidentTopologyToPolygonOffset()
edgeActor = svtk.svtkActor()
edgeActor.SetMapper(edgeMapper)
edgeActor.GetProperty().SetColor(0, 0, 0)

## Test clipping
# Create the contour bands.
bcf2 = svtk.svtkBandedPolyDataContourFilter()
bcf2.SetInputConnection(warp.GetOutputPort())
bcf2.ClippingOn()
bcf2.GenerateValues(10, 1000, 2000)
bcf2.SetScalarModeToValue()

# Compute normals to give a better look.
normals2 = svtk.svtkPolyDataNormals()
normals2.SetInputConnection(bcf2.GetOutputPort())
normals2.SetFeatureAngle(60)
normals2.ConsistencyOff()
normals2.SplittingOff()

lut = svtk.svtkLookupTable()
lut.SetNumberOfColors(10)
demMapper2 = svtk.svtkPolyDataMapper()
demMapper2.SetInputConnection(normals2.GetOutputPort())
demMapper2.SetScalarRange(demModel.GetOutput().GetScalarRange())
demMapper2.SetLookupTable(lut)
demMapper2.SetScalarModeToUseCellData()

demActor2 = svtk.svtkLODActor()
demActor2.SetMapper(demMapper2)
demActor2.AddPosition(0, 15000, 0)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(demActor)
ren.AddActor(demActor2)
ren.AddActor(edgeActor)

ren.SetBackground(.4, .4, .4)
renWin.SetSize(375, 200)

cam = svtk.svtkCamera()
cam.SetPosition(-17438.8, 2410.62, 25470.8)
cam.SetFocalPoint(3985.35, 11930.6, 5922.14)
cam.SetViewUp(0, 0, 1)
ren.SetActiveCamera(cam)
ren.ResetCamera()
cam.Zoom(2)

iren.Initialize()
iren.SetDesiredUpdateRate(1)

def CheckAbort(obj, event):
    foo = renWin.GetEventPending()
    if foo != 0:
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", CheckAbort)
renWin.Render()

renWin.Render()
iren.Start()
