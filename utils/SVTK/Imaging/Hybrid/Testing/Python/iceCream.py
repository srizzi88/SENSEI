#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    svtk.svtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create implicit function primitives
cone = svtk.svtkCone()
cone.SetAngle(20)

vertPlane = svtk.svtkPlane()
vertPlane.SetOrigin(.1, 0, 0)
vertPlane.SetNormal(-1, 0, 0)

basePlane = svtk.svtkPlane()
basePlane.SetOrigin(1.2, 0, 0)
basePlane.SetNormal(1, 0, 0)

iceCream = svtk.svtkSphere()
iceCream.SetCenter(1.333, 0, 0)
iceCream.SetRadius(0.5)

bite = svtk.svtkSphere()
bite.SetCenter(1.5, 0, 0.5)
bite.SetRadius(0.25)

# combine primitives to build ice-cream cone
theCone = svtk.svtkImplicitBoolean()
theCone.SetOperationTypeToIntersection()
theCone.AddFunction(cone)
theCone.AddFunction(vertPlane)
theCone.AddFunction(basePlane)

theCream = svtk.svtkImplicitBoolean()
theCream.SetOperationTypeToDifference()
theCream.AddFunction(iceCream)
theCream.AddFunction(bite)

# iso-surface to create geometry
theConeSample = svtk.svtkSampleFunction()
theConeSample.SetImplicitFunction(theCone)
theConeSample.SetModelBounds(-1, 1.5, -1.25, 1.25, -1.25, 1.25)
theConeSample.SetSampleDimensions(60, 60, 60)
theConeSample.ComputeNormalsOff()

theConeSurface = svtk.svtkMarchingContourFilter()
theConeSurface.SetInputConnection(theConeSample.GetOutputPort())
theConeSurface.SetValue(0, 0.0)

coneMapper = svtk.svtkPolyDataMapper()
coneMapper.SetInputConnection(theConeSurface.GetOutputPort())
coneMapper.ScalarVisibilityOff()

coneActor = svtk.svtkActor()
coneActor.SetMapper(coneMapper)
coneActor.GetProperty().SetColor(GetRGBColor('chocolate'))

# iso-surface to create geometry
theCreamSample = svtk.svtkSampleFunction()
theCreamSample.SetImplicitFunction(theCream)
theCreamSample.SetModelBounds(0, 2.5, -1.25, 1.25, -1.25, 1.25)
theCreamSample.SetSampleDimensions(60, 60, 60)
theCreamSample.ComputeNormalsOff()

theCreamSurface = svtk.svtkMarchingContourFilter()
theCreamSurface.SetInputConnection(theCreamSample.GetOutputPort())
theCreamSurface.SetValue(0, 0.0)

creamMapper = svtk.svtkPolyDataMapper()
creamMapper.SetInputConnection(theCreamSurface.GetOutputPort())
creamMapper.ScalarVisibilityOff()

creamActor = svtk.svtkActor()
creamActor.SetMapper(creamMapper)
creamActor.GetProperty().SetColor(GetRGBColor('mint'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(coneActor)
ren1.AddActor(creamActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(250, 250)

ren1.ResetCamera()
ren1.GetActiveCamera().Roll(90)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
