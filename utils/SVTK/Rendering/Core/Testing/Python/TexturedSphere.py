#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

#
# Texture a sphere.
#

# renderer and interactor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read the volume
reader = svtk.svtkJPEGReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/beach.jpg")

#---------------------------------------------------------
# Do the surface rendering
sphereSource = svtk.svtkSphereSource()
sphereSource.SetRadius(100)

textureSphere = svtk.svtkTextureMapToSphere()
textureSphere.SetInputConnection(sphereSource.GetOutputPort())

sphereStripper = svtk.svtkStripper()
sphereStripper.SetInputConnection(textureSphere.GetOutputPort())
sphereStripper.SetMaximumLength(5)

sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(sphereStripper.GetOutputPort())
sphereMapper.ScalarVisibilityOff()

sphereTexture = svtk.svtkTexture()
sphereTexture.SetInputConnection(reader.GetOutputPort())
sphereProperty = svtk.svtkProperty()
# sphereProperty.BackfaceCullingOn()

sphere = svtk.svtkActor()
sphere.SetMapper(sphereMapper)
sphere.SetTexture(sphereTexture)
sphere.SetProperty(sphereProperty)

#---------------------------------------------------------
ren.AddViewProp(sphere)

camera = ren.GetActiveCamera()
camera.SetFocalPoint(0, 0, 0)
camera.SetPosition(100, 400, -100)
camera.SetViewUp(0, 0, -1)

ren.ResetCameraClippingRange()
renWin.Render()
#---------------------------------------------------------
# test-related code
def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)

iren.Initialize()
#iren.Start()
