#!/usr/bin/env python
import svtk
from math import cos, sin, pi
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
angle = pi/2
direction = [
  -1, 0, 0,
  0, cos(angle), -sin(angle),
  0, -sin(angle), -cos(angle)
]
reader = svtk.svtkImageReader2()
reader.SetDataScalarTypeToUnsignedShort()
reader.SetFilePrefix(SVTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataExtent(0, 63, 0, 63, 1, 93)
reader.SetDataSpacing(3.2, 3.2, 1.5)
reader.SetDataOrigin(0.0, 0.0, 0.0)
reader.SetDataDirection(direction)

iso = svtk.svtkImageMarchingCubes()
iso.SetInputConnection(reader.GetOutputPort())
iso.SetValue(0, 1150)
iso.SetInputMemoryLimit(100)

isoMapper = svtk.svtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = svtk.svtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('antique_white'))

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())

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
renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
