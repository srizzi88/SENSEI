#!/usr/bin/env python

# This example demonstrates how to use the svtkSphereWidget to control
# the position of a light.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Start by loading some data.
dem = svtk.svtkDEMReader()
dem.SetFileName(SVTK_DATA_ROOT + "/Data/SainteHelens.dem")
dem.Update()

Scale = 2
lut = svtk.svtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

lo = Scale*dem.GetElevationBounds()[0]
hi = Scale*dem.GetElevationBounds()[1]

shrink = svtk.svtkImageShrink3D()
shrink.SetShrinkFactors(4, 4, 1)
shrink.SetInputConnection(dem.GetOutputPort())
shrink.AveragingOn()

geom = svtk.svtkImageDataGeometryFilter()
geom.SetInputConnection(shrink.GetOutputPort())
geom.ReleaseDataFlagOn()

warp = svtk.svtkWarpScalar()
warp.SetInputConnection(geom.GetOutputPort())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(Scale)
warp.ReleaseDataFlagOn()

elevation = svtk.svtkElevationFilter()
elevation.SetInputConnection(warp.GetOutputPort())
elevation.SetLowPoint(0, 0, lo)
elevation.SetHighPoint(0, 0, hi)
elevation.SetScalarRange(lo, hi)
elevation.ReleaseDataFlagOn()

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(elevation.GetOutputPort())
normals.SetFeatureAngle(60)
normals.ConsistencyOff()
normals.SplittingOff()
normals.ReleaseDataFlagOn()

demMapper = svtk.svtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

demActor = svtk.svtkLODActor()
demActor.SetMapper(demMapper)

# Create the RenderWindow, Renderer and both Actors
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.LightFollowCameraOff()
iren.SetInteractorStyle(None)

# Associate the line widget with the interactor
sphereWidget = svtk.svtkSphereWidget()
sphereWidget.SetInteractor(iren)
sphereWidget.SetProp3D(demActor)
sphereWidget.SetPlaceFactor(4)
sphereWidget.PlaceWidget()
sphereWidget.TranslationOff()
sphereWidget.ScaleOff()
sphereWidget.HandleVisibilityOn()

# Uncomment the next line if you want to see the widget active when
# the script starts
#sphereWidget.EnabledOn()

# Actually probe the data
def MoveLight(obj, event):
    global light
    light.SetPosition(obj.GetHandlePosition())

sphereWidget.AddObserver("InteractionEvent", MoveLight)

# Add the actors to the renderer, set the background and size
ren.AddActor(demActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetViewUp(0, 0, 1)
cam1.SetFocalPoint(dem.GetOutput().GetCenter())
cam1.SetPosition(1, 0, 0)
ren.ResetCamera()
cam1.Elevation(25)
cam1.Azimuth(125)
cam1.Zoom(1.25)

light = svtk.svtkLight()
light.SetFocalPoint(dem.GetOutput().GetCenter())
ren.AddLight(light)

iren.Initialize()
renWin.Render()
iren.Start()
