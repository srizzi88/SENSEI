#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
dilate = svtk.svtkImageContinuousDilate3D()
dilate.SetInputConnection(reader.GetOutputPort())
dilate.SetKernelSize(11,11,1)
erode = svtk.svtkImageContinuousErode3D()
erode.SetInputConnection(dilate.GetOutputPort())
erode.SetKernelSize(11,11,1)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(erode.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
