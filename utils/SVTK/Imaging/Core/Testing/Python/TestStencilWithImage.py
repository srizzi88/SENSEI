#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test the stencil filter by using one image
# to stencil another
# Image pipeline
reader1 = svtk.svtkBMPReader()
reader1.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
reader2 = svtk.svtkPNMReader()
reader2.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/B.pgm")
translate = svtk.svtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60,60,0)
imageToStencil = svtk.svtkImageToImageStencil()
imageToStencil.SetInputConnection(translate.GetOutputPort())
imageToStencil.ThresholdBetween(0,127)
# silly stuff to increase coverage
imageToStencil.SetUpperThreshold(imageToStencil.GetUpperThreshold())
imageToStencil.SetLowerThreshold(imageToStencil.GetLowerThreshold())
stencil = svtk.svtkImageStencil()
stencil.SetInputConnection(reader1.GetOutputPort())
stencil.SetBackgroundValue(0)
stencil.ReverseStencilOn()
stencil.SetStencilConnection(imageToStencil.GetOutputPort())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(stencil.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
# --- end of script --
