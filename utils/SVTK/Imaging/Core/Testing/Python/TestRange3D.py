#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
range = svtk.svtkImageRange3D()
range.SetInputConnection(reader.GetOutputPort())
range.SetKernelSize(5,5,5)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(range.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
#viewer DebugOn
viewer.Render()
# --- end of script --
