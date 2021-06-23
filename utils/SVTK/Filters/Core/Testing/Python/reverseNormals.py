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

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cowReader = svtk.svtkOBJReader()
cowReader.SetFileName(SVTK_DATA_ROOT + "/Data/Viewpoint/cow.obj")

plane = svtk.svtkPlane()
plane.SetNormal(1, 0, 0)

cowClipper = svtk.svtkClipPolyData()
cowClipper.SetInputConnection(cowReader.GetOutputPort())
cowClipper.SetClipFunction(plane)

cellNormals = svtk.svtkPolyDataNormals()
cellNormals.SetInputConnection(cowClipper.GetOutputPort())
cellNormals.ComputePointNormalsOn()
cellNormals.ComputeCellNormalsOn()

reflect = svtk.svtkTransform()
reflect.Scale(-1, 1, 1)

cowReflect = svtk.svtkTransformPolyDataFilter()
cowReflect.SetTransform(reflect)
cowReflect.SetInputConnection(cellNormals.GetOutputPort())

cowReverse = svtk.svtkReverseSense()
cowReverse.SetInputConnection(cowReflect.GetOutputPort())
cowReverse.ReverseNormalsOn()
cowReverse.ReverseCellsOff()

reflectedMapper = svtk.svtkPolyDataMapper()
reflectedMapper.SetInputConnection(cowReverse.GetOutputPort())

reflected = svtk.svtkActor()
reflected.SetMapper(reflectedMapper)
reflected.GetProperty().SetDiffuseColor(GetRGBColor('flesh'))
reflected.GetProperty().SetDiffuse(.8)
reflected.GetProperty().SetSpecular(.5)
reflected.GetProperty().SetSpecularPower(30)
reflected.GetProperty().FrontfaceCullingOn()

ren1.AddActor(reflected)

cowMapper = svtk.svtkPolyDataMapper()
cowMapper.SetInputConnection(cowClipper.GetOutputPort())

cow = svtk.svtkActor()
cow.SetMapper(cowMapper)

ren1.AddActor(cow)

ren1.SetBackground(.1, .2, .4)

renWin.SetSize(320, 240)

ren1.ResetCamera()
ren1.GetActiveCamera().SetViewUp(0, 1, 0)
ren1.GetActiveCamera().Azimuth(180)
ren1.GetActiveCamera().Dolly(1.75)
ren1.ResetCameraClippingRange()

iren.Initialize()

# render the image
#iren.Start()
