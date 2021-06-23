#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Simple viewer for images.
# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
permute = svtk.svtkImagePermute()
permute.SetInputConnection(reader.GetOutputPort())
permute.SetFilteredAxes(1,2,0)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(permute.GetOutputPort())
viewer.SetZSlice(32)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer DebugOn
#viewer Render
#make interface
viewer.Render()
# --- end of script --
