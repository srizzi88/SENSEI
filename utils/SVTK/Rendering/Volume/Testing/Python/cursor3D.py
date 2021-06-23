#!/usr/bin/env python

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This little example shows how a cursor can be created in
# image viewers, and renderers.

# Global values

CURSOR_X = 20
CURSOR_Y = 20
CURSOR_Z = 20

IMAGE_MAG_X = 4
IMAGE_MAG_Y = 4
IMAGE_MAG_Z = 1

# Pipeline stuff

reader = svtk.svtkSLCReader()
reader.SetFileName(str(SVTK_DATA_ROOT) + "/Data/neghip.slc")

# Cursor stuff

magnify = svtk.svtkImageMagnify()
magnify.SetInputConnection(reader.GetOutputPort())
magnify.SetMagnificationFactors(IMAGE_MAG_X,IMAGE_MAG_Y,IMAGE_MAG_Z)

image_cursor = svtk.svtkImageCursor3D()
image_cursor.SetInputConnection(magnify.GetOutputPort())
image_cursor.SetCursorPosition(CURSOR_X*IMAGE_MAG_X,
                               CURSOR_Y*IMAGE_MAG_Y,
                               CURSOR_Z*IMAGE_MAG_Z)
image_cursor.SetCursorValue(255)
image_cursor.SetCursorRadius(50*IMAGE_MAG_X)

axes = svtk.svtkAxes()
axes.SymmetricOn()
axes.SetOrigin(CURSOR_X,CURSOR_Y,CURSOR_Z)
axes.SetScaleFactor(50.0)

axes_mapper = svtk.svtkPolyDataMapper()
axes_mapper.SetInputConnection(axes.GetOutputPort())

axesActor = svtk.svtkActor()
axesActor.SetMapper(axes_mapper)
axesActor.GetProperty().SetAmbient(0.5)

# Image viewer stuff

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(image_cursor.GetOutputPort())
viewer.SetZSlice(CURSOR_Z*IMAGE_MAG_Z)
viewer.SetColorWindow(256)
viewer.SetColorLevel(128)

# Create transfer functions for opacity and color

opacity_transfer_function = svtk.svtkPiecewiseFunction()
opacity_transfer_function.AddPoint(20, 0.0)
opacity_transfer_function.AddPoint(255, 0.2)

color_transfer_function = svtk.svtkColorTransferFunction()
color_transfer_function.AddRGBPoint(0, 0, 0, 0)
color_transfer_function.AddRGBPoint(64, 1, 0, 0)
color_transfer_function.AddRGBPoint(128, 0, 0, 1)
color_transfer_function.AddRGBPoint(192, 0, 1, 0)
color_transfer_function.AddRGBPoint(255, 0, .2, 0)

# Create properties, mappers, volume actors, and ray cast function

volume_property = svtk.svtkVolumeProperty()
volume_property.SetColor(color_transfer_function)
volume_property.SetScalarOpacity(opacity_transfer_function)

volume_mapper = svtk.svtkFixedPointVolumeRayCastMapper()
volume_mapper.SetInputConnection(reader.GetOutputPort())

volume = svtk.svtkVolume()
volume.SetMapper(volume_mapper)
volume.SetProperty(volume_property)

# Create outline

outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())

outline_mapper = svtk.svtkPolyDataMapper()
outline_mapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outline_mapper)
outlineActor.GetProperty().SetColor(1, 1, 1)

# Create the renderer

ren = svtk.svtkRenderer()
ren.AddActor(axesActor)
ren.AddVolume(volume)
ren.SetBackground(0.1, 0.2, 0.4)

renWin2 = svtk.svtkRenderWindow()
renWin2.AddRenderer(ren)
renWin2.SetSize(256, 256)

renWin2.Render()
