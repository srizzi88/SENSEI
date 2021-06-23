#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0, 63, 0, 63, 1, 93)
reader.SetFilePrefix(SVTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)

mag = svtk.svtkImageMagnify()
mag.SetInputConnection(reader.GetOutputPort())
mag.SetMagnificationFactors(4, 4, 1)

th = svtk.svtkImageThreshold()
th.SetInputConnection(mag.GetOutputPort())
th.SetReplaceIn(1)
th.SetReplaceOut(1)
th.ThresholdBetween(-1000, 1000)
th.SetOutValue(0)
th.SetInValue(2000)

cast = svtk.svtkImageCast()
cast.SetInputConnection(mag.GetOutputPort())
cast.SetOutputScalarTypeToFloat()

cast2 = svtk.svtkImageCast()
cast2.SetInputConnection(th.GetOutputPort())
cast2.SetOutputScalarTypeToFloat()

sum = svtk.svtkImageWeightedSum()
sum.AddInputConnection(cast.GetOutputPort())
sum.AddInputConnection(cast2.GetOutputPort())
sum.SetWeight(0, 10)
sum.SetWeight(1, 4)

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(sum.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(1819)
viewer.SetColorLevel(939)
sum.SetWeight(0, 1)
# make interface
viewer.SetSize(256, 256)
viewer.Render()
