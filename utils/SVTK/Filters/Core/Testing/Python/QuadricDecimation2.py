#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

res = 100
# Test the volume constraint. Create "planar" data with jittery
# z-direction coordinates. Decimate the data with and without
# planar constraints and compare.
#
ps = svtk.svtkPlaneSource()
ps.SetResolution(res,res)

tf = svtk.svtkTriangleFilter()
tf.SetInputConnection(ps.GetOutputPort())

attr = svtk.svtkRandomAttributeGenerator()
attr.SetInputConnection(tf.GetOutputPort())
attr.GeneratePointScalarsOn()
attr.SetMinimumComponentValue(-0.05)
attr.SetMaximumComponentValue(0.05)

# This jitters the geometry
warp = svtk.svtkWarpScalar()
warp.SetInputConnection(attr.GetOutputPort())
warp.SetScaleFactor(0.02)

# Decimator without volume constraint
deci = svtk.svtkQuadricDecimation()
deci.SetInputConnection(warp.GetOutputPort())
deci.SetTargetReduction(.95)
deci.AttributeErrorMetricOn()
deci.VolumePreservationOff()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(warp.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Decimator with volume constraint
deci2 = svtk.svtkQuadricDecimation()
deci2.SetInputConnection(warp.GetOutputPort())
deci2.SetTargetReduction(.95)
deci2.AttributeErrorMetricOn()
deci2.VolumePreservationOn()

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(deci.GetOutputPort())

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)

# Original noisy surface
#
mapper3 = svtk.svtkPolyDataMapper()
mapper3.SetInputConnection(deci2.GetOutputPort())

actor3 = svtk.svtkActor()
actor3.SetMapper(mapper3)

# Create rendering instances
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,.33,1)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.33,0,0.66,1)
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.66,0,1,1)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Set up the camera parameters
#
camera = svtk.svtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(0,0,1)
camera.Elevation(45.0)
ren0.SetActiveCamera(camera)
ren1.SetActiveCamera(camera)
ren2.SetActiveCamera(camera)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(actor)
ren1.AddActor(actor2)
ren2.AddActor(actor3)
ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)
renWin.SetSize(600,300)
ren0.ResetCamera()
renWin.Render()
iren.Initialize()

# gather some information
print ( "Bounds (volume preserve off): ({0})".format( deci.GetOutput().GetPoints().GetBounds() ) )
print ( "Bounds (volume preserve on): ({0})".format( deci2.GetOutput().GetPoints().GetBounds() ) )

#iren.Start()
# --- end of script --
