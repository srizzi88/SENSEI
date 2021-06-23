#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.
# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
sobel = svtk.svtkImageSobel3D()
sobel.SetInputConnection(reader.GetOutputPort())
sobel.ReleaseDataFlagOff()
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(sobel.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
