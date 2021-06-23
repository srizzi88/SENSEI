#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Shift and scale an image (in that order)
# This filter is useful for converting to a lower precision data type.
reader = svtk.svtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
shiftScale = svtk.svtkImageShiftScale()
shiftScale.SetInputConnection(reader.GetOutputPort())
shiftScale.SetShift(0)
shiftScale.SetScale(0.5)
shiftScale.SetOutputScalarTypeToDouble()
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
viewer.SetColorLevel(1024)
#make interface
#skipping source
#svtkPNMWriter w
#w SetFileName "D:/svtknew/svtk/graphics/examplesTcl/mace2.ppm"
#w SetInputConnection [shiftScale GetOutputPort]
#w Write
# --- end of script --
