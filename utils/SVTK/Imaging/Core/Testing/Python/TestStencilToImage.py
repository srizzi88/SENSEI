#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test converting a stencil to a binary image
sphere = svtk.svtkSphere()
sphere.SetCenter(128,128,0)
sphere.SetRadius(80)
functionToStencil = svtk.svtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.SetOutputOrigin(0,0,0)
functionToStencil.SetOutputSpacing(1,1,1)
functionToStencil.SetOutputWholeExtent(0,255,0,255,0,0)
stencilToImage = svtk.svtkImageStencilToImage()
stencilToImage.SetInputConnection(functionToStencil.GetOutputPort())
stencilToImage.SetOutsideValue(0)
stencilToImage.SetInsideValue(255)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(stencilToImage.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
