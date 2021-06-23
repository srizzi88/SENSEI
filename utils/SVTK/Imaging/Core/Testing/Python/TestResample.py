#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Doubles the number of images (x dimension).
# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
magnify = svtk.svtkImageResample()
magnify.SetDimensionality(3)
magnify.SetInputConnection(reader.GetOutputPort())
magnify.SetAxisOutputSpacing(0,0.52)
magnify.SetAxisOutputSpacing(1,2.2)
magnify.SetAxisOutputSpacing(2,0.8)
magnify.ReleaseDataFlagOff()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(magnify.GetOutputPort())
viewer.SetZSlice(30)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
