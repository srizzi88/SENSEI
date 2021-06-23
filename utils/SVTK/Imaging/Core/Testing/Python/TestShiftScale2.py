#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Shift and scale an image (in that order)
# This filter is useful for converting to a lower precision data type.
# This script tests the clamp overflow feature.
reader = svtk.svtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
shiftScale = svtk.svtkImageShiftScale()
shiftScale.SetInputConnection(reader.GetOutputPort())
shiftScale.SetShift(-1000.0)
shiftScale.SetScale(4.0)
shiftScale.SetOutputScalarTypeToUnsignedShort()
shiftScale.ClampOverflowOn()
shiftScale2 = svtk.svtkImageShiftScale()
shiftScale2.SetInputConnection(shiftScale.GetOutputPort())
shiftScale2.SetShift(0)
shiftScale2.SetScale(2.0)
mag = svtk.svtkImageMagnify()
mag.SetInputConnection(shiftScale2.GetOutputPort())
mag.SetMagnificationFactors(4,4,1)
mag.InterpolateOff()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(mag.GetOutputPort())
viewer.SetColorWindow(1024)
viewer.SetColorLevel(512)
#make interface
#skipping source
# --- end of script --
