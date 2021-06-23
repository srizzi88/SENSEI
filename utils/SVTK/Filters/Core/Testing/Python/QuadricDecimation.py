#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# pipeline stuff
#
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(SVTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(SVTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)
gf = svtk.svtkGeometryFilter()
gf.SetInputData(pl3d_output)
tf = svtk.svtkTriangleFilter()
tf.SetInputConnection(gf.GetOutputPort())
gMapper = svtk.svtkPolyDataMapper()
gMapper.SetInputConnection(gf.GetOutputPort())
gMapper.SetScalarRange(pl3d_output.GetScalarRange())
gActor = svtk.svtkActor()
gActor.SetMapper(gMapper)
# Don't look at attributes
mesh = svtk.svtkQuadricDecimation()
mesh.SetInputConnection(tf.GetOutputPort())
mesh.SetTargetReduction(.90)
mesh.AttributeErrorMetricOn()
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(mesh.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# This time worry about attributes
mesh2 = svtk.svtkQuadricDecimation()
mesh2.SetInputConnection(tf.GetOutputPort())
mesh2.SetTargetReduction(.90)
mesh2.AttributeErrorMetricOff()
mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(mesh2.GetOutputPort())
actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.AddPosition(0,12,0)
# Create rendering instances
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Set up the camera parameters
#
camera = svtk.svtkCamera()
camera.SetPosition(19.34,6.128,-11.96)
camera.SetFocalPoint(8.25451,6.0,29.77)
camera.SetViewUp(0.9664,0.00605,0.256883)
camera.SetViewAngle(30)
camera.SetClippingRange(26,64)
ren1.SetActiveCamera(camera)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(actor2)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
threshold = 50
# --- end of script --
