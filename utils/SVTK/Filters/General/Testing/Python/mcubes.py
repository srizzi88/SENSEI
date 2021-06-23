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

# create pipeline
#
v16 = svtk.svtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.GetOutput().SetOrigin(0.0, 0.0, 0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(SVTK_DATA_ROOT + "/Data/headsq/quarter")
v16.SetImageRange(1, 93)
v16.SetDataSpacing(3.2, 3.2, 1.5)
v16.Update()

iso = svtk.svtkMarchingContourFilter()
iso.SetInputConnection(v16.GetOutputPort())
iso.SetValue(0, 1125)

isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('antique_white'))

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0.2, 0.3, 0.4)

renWin.SetSize(200, 200)

ren1.ResetCamera()
ren1.GetActiveCamera().Elevation(90)
ren1.GetActiveCamera().SetViewUp(0, 0, -1)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

renWin.Render()

#iren.Start()
