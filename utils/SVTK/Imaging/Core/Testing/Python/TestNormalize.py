#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script is for testing the normalize filter.
# Image pipeline
cos = svtk.svtkImageSinusoidSource()
cos.SetWholeExtent(0,225,0,225,0,20)
cos.SetAmplitude(250)
cos.SetDirection(1,1,1)
cos.SetPeriod(20)
cos.ReleaseDataFlagOff()
gradient = svtk.svtkImageGradient()
gradient.SetInputConnection(cos.GetOutputPort())
gradient.SetDimensionality(3)
norm = svtk.svtkImageNormalize()
norm.SetInputConnection(gradient.GetOutputPort())
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(norm.GetOutputPort())
viewer.SetZSlice(10)
viewer.SetColorWindow(2.0)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
