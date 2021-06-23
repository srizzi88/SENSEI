#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test the stencil filter.
# removes all but a sphere.
reader = svtk.svtkPNGReader()
reader.SetDataSpacing(0.8,0.8,1.5)
reader.SetDataOrigin(0.0,0.0,0.0)
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
reader.Update()
sphere = svtk.svtkSphere()
sphere.SetCenter(128,128,0)
sphere.SetRadius(80)
functionToStencil = svtk.svtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.SetInformationInput(reader.GetOutput())
functionToStencil.Update()
# test making a copying of the stencil (for coverage)
stencilOriginal = functionToStencil.GetOutput()
stencilCopy = stencilOriginal.NewInstance()
stencilCopy.DeepCopy(functionToStencil.GetOutput())
shiftScale = svtk.svtkImageShiftScale()
shiftScale.SetInputConnection(reader.GetOutputPort())
shiftScale.SetScale(0.2)
shiftScale.Update()
stencil = svtk.svtkImageStencil()
stencil.SetInputConnection(reader.GetOutputPort())
stencil.SetBackgroundInputData(shiftScale.GetOutput())
stencil.SetStencilData(stencilCopy)
stencilCopy.UnRegister(stencil) # not needed in python
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(stencil.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
