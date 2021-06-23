#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# to mark the origin
sphere = svtk.svtkSphereSource()
sphere.SetRadius(2.0)

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)

rt = svtk.svtkRTAnalyticSource()
rt.SetWholeExtent(-40, 60, -25, 75, 0, 0)
rt.Update()
im = rt.GetOutput()
im.SetDirectionMatrix(-1, 0, 0, 0, -1, 0, 0, 0, 1)

voi = svtk.svtkExtractVOI()
voi.SetInputData(im)
voi.SetVOI(-11, 39, 5, 45, 0, 0)
voi.SetSampleRate(5, 5, 1)

# Get rid of ambiguous triagulation issues.
surf = svtk.svtkDataSetSurfaceFilter()
surf.SetInputConnection(voi.GetOutputPort())

tris = svtk.svtkTriangleFilter()
tris.SetInputConnection(surf.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(tris.GetOutputPort())
mapper.SetScalarRange(130, 280)

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren = svtk.svtkRenderer()
ren.AddActor(actor)
ren.AddActor(sphereActor)
ren.ResetCamera()

# camera = ren.GetActiveCamera()
# camera.SetPosition(68.1939, -23.4323, 12.6465)
# camera.SetViewUp(0.46563, 0.882375, 0.0678508)
# camera.SetFocalPoint(3.65707, 11.4552, 1.83509)
# camera.SetClippingRange(59.2626, 101.825)

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)

dm = voi.GetOutput().GetDirectionMatrix()
if dm.GetElement(0, 0) != -1 or dm.GetElement(1, 1) != -1 or dm.GetElement(2, 2) != 1:
	print("ERROR: svtkExtractVOI not passing DirectionMatrix unchanged")

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
#iren.Start()
