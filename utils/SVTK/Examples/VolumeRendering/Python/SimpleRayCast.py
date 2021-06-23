#!/usr/bin/env python

# This is a simple volume rendering example that uses a
# svtkGPUVolumeRayCastMapper

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the standard renderer, render window and interactor
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create the reader for the data
reader = svtk.svtkStructuredPointsReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/ironProt.svtk")

# Create transfer mapping scalar value to opacity
opacityTransferFunction = svtk.svtkPiecewiseFunction()
opacityTransferFunction.AddPoint(20, 0.0)
opacityTransferFunction.AddPoint(255, 0.2)

# Create transfer mapping scalar value to color
colorTransferFunction = svtk.svtkColorTransferFunction()
colorTransferFunction.AddRGBPoint(0.0, 0.0, 0.0, 0.0)
colorTransferFunction.AddRGBPoint(64.0, 1.0, 0.0, 0.0)
colorTransferFunction.AddRGBPoint(128.0, 0.0, 0.0, 1.0)
colorTransferFunction.AddRGBPoint(192.0, 0.0, 1.0, 0.0)
colorTransferFunction.AddRGBPoint(255.0, 0.0, 0.2, 0.0)

# The property describes how the data will look
volumeProperty = svtk.svtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.ShadeOn()
volumeProperty.SetInterpolationTypeToLinear()

# The mapper / ray cast function know how to render the data
volumeMapper = svtk.svtkGPUVolumeRayCastMapper()
volumeMapper.SetBlendModeToComposite()
volumeMapper.SetInputConnection(reader.GetOutputPort())

# The volume holds the mapper and the property and
# can be used to position/orient the volume
volume = svtk.svtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

ren.AddVolume(volume)
ren.SetBackground(1, 1, 1)
renWin.SetSize(600, 600)
renWin.Render()

def CheckAbort(obj, event):
    if obj.GetEventPending() != 0:
        obj.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", CheckAbort)

iren.Initialize()
renWin.Render()
iren.Start()
