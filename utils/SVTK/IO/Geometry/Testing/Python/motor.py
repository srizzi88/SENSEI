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

# create cutting planes
planes = svtk.svtkPlanes()
points = svtk.svtkPoints()
norms = svtk.svtkFloatArray()

norms.SetNumberOfComponents(3)
points.InsertPoint(0, 0.0, 0.0, 0.0)
norms.InsertTuple3(0, 0.0, 0.0, 1.0)
points.InsertPoint(1, 0.0, 0.0, 0.0)
norms.InsertTuple3(1, -1.0, 0.0, 0.0)
planes.SetPoints(points)
planes.SetNormals(norms)

# texture
texReader = svtk.svtkStructuredPointsReader()
texReader.SetFileName(SVTK_DATA_ROOT + "/Data/texThres2.svtk")
texture = svtk.svtkTexture()
texture.SetInputConnection(texReader.GetOutputPort())
texture.InterpolateOff()
texture.RepeatOff()

# read motor parts...each part colored separately
#
byu = svtk.svtkBYUReader()
byu.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/motor.g")
byu.SetPartNumber(1)

normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(byu.GetOutputPort())

tex1 = svtk.svtkImplicitTextureCoords()
tex1.SetInputConnection(normals.GetOutputPort())
tex1.SetRFunction(planes)
# tex1.FlipTextureOn()

byuMapper = svtk.svtkDataSetMapper()
byuMapper.SetInputConnection(tex1.GetOutputPort())

byuActor = svtk.svtkActor()
byuActor.SetMapper(byuMapper)
byuActor.SetTexture(texture)
byuActor.GetProperty().SetColor(GetRGBColor('cold_grey'))

byu2 = svtk.svtkBYUReader()
byu2.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/motor.g")
byu2.SetPartNumber(2)

normals2 = svtk.svtkPolyDataNormals()
normals2.SetInputConnection(byu2.GetOutputPort())

tex2 = svtk.svtkImplicitTextureCoords()
tex2.SetInputConnection(normals2.GetOutputPort())
tex2.SetRFunction(planes)
# tex2.FlipTextureOn()

byuMapper2 = svtk.svtkDataSetMapper()
byuMapper2.SetInputConnection(tex2.GetOutputPort())

byuActor2 = svtk.svtkActor()
byuActor2.SetMapper(byuMapper2)
byuActor2.SetTexture(texture)
byuActor2.GetProperty().SetColor(GetRGBColor('peacock'))

byu3 = svtk.svtkBYUReader()
byu3.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/motor.g")
byu3.SetPartNumber(3)

triangle3 = svtk.svtkTriangleFilter()
triangle3.SetInputConnection(byu3.GetOutputPort())

normals3 = svtk.svtkPolyDataNormals()
normals3.SetInputConnection(triangle3.GetOutputPort())

tex3 = svtk.svtkImplicitTextureCoords()
tex3.SetInputConnection(normals3.GetOutputPort())
tex3.SetRFunction(planes)
# tex3.FlipTextureOn()

byuMapper3 = svtk.svtkDataSetMapper()
byuMapper3.SetInputConnection(tex3.GetOutputPort())

byuActor3 = svtk.svtkActor()
byuActor3.SetMapper(byuMapper3)
byuActor3.SetTexture(texture)
byuActor3.GetProperty().SetColor(GetRGBColor('raw_sienna'))

byu4 = svtk.svtkBYUReader()
byu4.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/motor.g")
byu4.SetPartNumber(4)

normals4 = svtk.svtkPolyDataNormals()
normals4.SetInputConnection(byu4.GetOutputPort())

tex4 = svtk.svtkImplicitTextureCoords()
tex4.SetInputConnection(normals4.GetOutputPort())
tex4.SetRFunction(planes)
# tex4.FlipTextureOn()

byuMapper4 = svtk.svtkDataSetMapper()
byuMapper4.SetInputConnection(tex4.GetOutputPort())

byuActor4 = svtk.svtkActor()
byuActor4.SetMapper(byuMapper4)
byuActor4.SetTexture(texture)
byuActor4.GetProperty().SetColor(GetRGBColor('banana'))

byu5 = svtk.svtkBYUReader()
byu5.SetGeometryFileName(SVTK_DATA_ROOT + "/Data/motor.g")
byu5.SetPartNumber(5)

normals5 = svtk.svtkPolyDataNormals()
normals5.SetInputConnection(byu5.GetOutputPort())

tex5 = svtk.svtkImplicitTextureCoords()
tex5.SetInputConnection(normals5.GetOutputPort())
tex5.SetRFunction(planes)
# tex5.FlipTextureOn()

byuMapper5 = svtk.svtkDataSetMapper()
byuMapper5.SetInputConnection(tex5.GetOutputPort())

byuActor5 = svtk.svtkActor()
byuActor5.SetMapper(byuMapper5)
byuActor5.SetTexture(texture)
byuActor5.GetProperty().SetColor(GetRGBColor('peach_puff'))

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(byuActor)
ren1.AddActor(byuActor2)
ren1.AddActor(byuActor3)
byuActor3.VisibilityOff()
ren1.AddActor(byuActor4)
ren1.AddActor(byuActor5)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

camera = svtk.svtkCamera()
camera.SetFocalPoint(0.0286334, 0.0362996, 0.0379685)
camera.SetPosition(1.37067, 1.08629, -1.30349)
camera.SetViewAngle(17.673)
camera.SetClippingRange(1, 10)
camera.SetViewUp(-0.376306, -0.5085, -0.774482)
ren1.SetActiveCamera(camera)

# render the image
iren.Initialize()
#iren.Start()
