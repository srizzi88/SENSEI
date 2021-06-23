#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = svtk.svtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
fft.ReleaseDataFlagOff()
#fft DebugOn
magnitude = svtk.svtkImageMagnitude()
magnitude.SetInputConnection(fft.GetOutputPort())
magnitude.ReleaseDataFlagOff()
center = svtk.svtkImageFourierCenter()
center.SetInputConnection(magnitude.GetOutputPort())
compress = svtk.svtkImageLogarithmicScale()
compress.SetInputConnection(center.GetOutputPort())
compress.SetConstant(15)
viewer = svtk.svtkImageViewer2()
viewer.SetInputConnection(compress.GetOutputPort())
viewer.SetColorWindow(150)
viewer.SetColorLevel(170)
viewInt = svtk.svtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()
# --- end of script --
