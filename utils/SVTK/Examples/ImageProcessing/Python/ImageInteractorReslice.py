#!/usr/bin/env python

# A simple svtkInteractorStyleImage example for
# 3D image viewing with the svtkImageResliceMapper.

# Drag Left mouse button to window/level
# Shift-Left drag to rotate (oblique slice)
# Shift-Middle drag to slice through image
# OR Ctrl-Right drag to slice through image

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataMask(0x7fff)
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")

# Create the RenderWindow, Renderer
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

im = svtk.svtkImageResliceMapper()
im.SetInputConnection(reader.GetOutputPort())
im.SliceFacesCameraOn()
im.SliceAtFocalPointOn()
im.BorderOff()

ip = svtk.svtkImageProperty()
ip.SetColorWindow(2000)
ip.SetColorLevel(1000)
ip.SetAmbient(0.0)
ip.SetDiffuse(1.0)
ip.SetOpacity(1.0)
ip.SetInterpolationTypeToLinear()

ia = svtk.svtkImageSlice()
ia.SetMapper(im)
ia.SetProperty(ip)

ren1.AddViewProp(ia)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)

iren = svtk.svtkRenderWindowInteractor()
style = svtk.svtkInteractorStyleImage()
style.SetInteractionModeToImage3D()
iren.SetInteractorStyle(style)
renWin.SetInteractor(iren)

# render the image
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.ParallelProjectionOn()
ren1.ResetCameraClippingRange()
renWin.Render()

iren.Start()
