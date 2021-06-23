#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows the result of an ideal lowpass filter in frequency space.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = svtk.svtkImageFFT()
fft.SetDimensionality(2)
fft.SetInputConnection(reader.GetOutputPort())
#fft DebugOn
lowPass = svtk.svtkImageButterworthLowPass()
lowPass.SetInputConnection(fft.GetOutputPort())
lowPass.SetOrder(2)
lowPass.SetXCutOff(0.2)
lowPass.SetYCutOff(0.1)
lowPass.ReleaseDataFlagOff()
#lowPass DebugOn
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(lowPass.GetOutputPort())
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer DebugOn
viewer.Render()
# --- end of script --
