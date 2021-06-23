#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create renderer stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
cow = svtk.svtkBYUReader()
cow.SetGeometryFileName("" + str(SVTK_DATA_ROOT) + "/Data/Viewpoint/cow.g")
cowMapper = svtk.svtkPolyDataMapper()
cowMapper.SetInputConnection(cow.GetOutputPort())
cowActor = svtk.svtkActor()
cowActor.SetMapper(cowMapper)
cowActor.GetProperty().SetDiffuseColor(0.9608,0.8706,0.7020)
cowAxesSource = svtk.svtkAxes()
cowAxesSource.SetScaleFactor(10)
cowAxesSource.SetOrigin(0,0,0)
cowAxesMapper = svtk.svtkPolyDataMapper()
cowAxesMapper.SetInputConnection(cowAxesSource.GetOutputPort())
cowAxes = svtk.svtkActor()
cowAxes.SetMapper(cowAxesMapper)
ren1.AddActor(cowAxes)
cowAxes.VisibilityOff()
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cowActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(320,240)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(0)
ren1.GetActiveCamera().Dolly(1.4)
ren1.ResetCameraClippingRange()
cowAxes.VisibilityOn()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
#
def RotateX (__svtk__temp0=0,__svtk__temp1=0):
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateX(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateY (__svtk__temp0=0,__svtk__temp1=0):
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateY(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateZ (__svtk__temp0=0,__svtk__temp1=0):
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateZ(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateXY (__svtk__temp0=0,__svtk__temp1=0):
    cowActor.SetOrientation(0,0,0)
    cowActor.RotateX(60)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateY(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

RotateX()
RotateY()
RotateZ()
RotateXY()
renWin.EraseOff()
# --- end of script --
