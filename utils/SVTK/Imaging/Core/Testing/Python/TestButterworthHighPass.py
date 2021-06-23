#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows the result of an ideal highpass filter in frequency space.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = svtk.svtkImageFFT()
fft.SetDimensionality(2)
fft.SetInputConnection(reader.GetOutputPort())
#fft DebugOn
highPass = svtk.svtkImageButterworthHighPass()
highPass.SetInputConnection(fft.GetOutputPort())
highPass.SetOrder(2)
highPass.SetXCutOff(0.2)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
#highPass DebugOn
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(highPass.GetOutputPort())
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer DebugOn
viewer.Render()
# --- end of script --
