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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

aTriangularTexture = svtk.svtkTriangularTexture()
aTriangularTexture.SetTexturePattern(2)
aTriangularTexture.SetScaleFactor(1.3)
aTriangularTexture.SetXSize(64)
aTriangularTexture.SetYSize(64)

aSphere = svtk.svtkSphereSource()
aSphere.SetThetaResolution(20)
aSphere.SetPhiResolution(20)

tCoords = svtk.svtkTriangularTCoords()
tCoords.SetInputConnection(aSphere.GetOutputPort())

triangleMapper = svtk.svtkPolyDataMapper()
triangleMapper.SetInputConnection(tCoords.GetOutputPort())

aTexture = svtk.svtkTexture()
aTexture.SetInputConnection(aTriangularTexture.GetOutputPort())
aTexture.InterpolateOn()

texturedActor = svtk.svtkActor()
texturedActor.SetMapper(triangleMapper)
texturedActor.SetTexture(aTexture)
texturedActor.GetProperty().BackfaceCullingOn()
texturedActor.GetProperty().SetDiffuseColor(GetRGBColor('banana'))
texturedActor.GetProperty().SetSpecular(.4)
texturedActor.GetProperty().SetSpecularPower(40)

aCube = svtk.svtkCubeSource()
aCube.SetXLength(.5)
aCube.SetYLength(.5)

aCubeMapper = svtk.svtkPolyDataMapper()
aCubeMapper.SetInputConnection(aCube.GetOutputPort())

cubeActor = svtk.svtkActor()
cubeActor.SetMapper(aCubeMapper)
cubeActor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))

ren1.SetBackground(GetRGBColor('slate_grey'))
ren1.AddActor(cubeActor)
ren1.AddActor(texturedActor)

ren1.ResetCamera()

ren1.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()
#iren.Start()
