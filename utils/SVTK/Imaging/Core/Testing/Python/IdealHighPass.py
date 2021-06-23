#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows the result of an ideal highpass filter in spatial domain
# Image pipeline
createReader = svtk.svtkImageReader2Factory()
reader = createReader.CreateImageReader2("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = svtk.svtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
highPass = svtk.svtkImageIdealHighPass()
highPass.SetInputConnection(fft.GetOutputPort())
highPass.SetXCutOff(0.1)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
rfft = svtk.svtkImageRFFT()
rfft.SetInputConnection(highPass.GetOutputPort())
real = svtk.svtkImageExtractComponents()
real.SetInputConnection(rfft.GetOutputPort())
real.SetComponents(0)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(real.GetOutputPort())
viewer.SetColorWindow(500)
viewer.SetColorLevel(0)
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
