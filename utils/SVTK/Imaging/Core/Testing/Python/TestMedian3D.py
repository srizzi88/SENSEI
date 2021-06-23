#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Test the median filter.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
median = svtk.svtkImageMedian3D()
median.SetInputConnection(reader.GetOutputPort())
median.SetKernelSize(7,7,1)
median.ReleaseDataFlagOff()
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(median.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
