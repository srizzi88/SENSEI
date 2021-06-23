#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
cache = svtk.svtkImageCacheFilter()
cache.SetInputConnection(reader.GetOutputPort())
cache.SetCacheSize(20)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(cache.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.SetPosition(50,50)
i = 0
while i < 5:
    j = 10
    while j < 30:
        viewer.SetZSlice(j)
        viewer.Render()
        j = j + 1

    i = i + 1

#make interface
viewer.Render()
# --- end of script --
