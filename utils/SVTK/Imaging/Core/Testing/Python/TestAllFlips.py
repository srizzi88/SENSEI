#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
imageFloat = svtk.svtkImageCast()
imageFloat.SetInputConnection(reader.GetOutputPort())
imageFloat.SetOutputScalarTypeToFloat()
flipX = svtk.svtkImageFlip()
flipX.SetInputConnection(imageFloat.GetOutputPort())
flipX.SetFilteredAxis(0)
flipY = svtk.svtkImageFlip()
flipY.SetInputConnection(imageFloat.GetOutputPort())
flipY.SetFilteredAxis(1)
flipY.FlipAboutOriginOn()
imageAppend = svtk.svtkImageAppend()
imageAppend.AddInputConnection(imageFloat.GetOutputPort())
imageAppend.AddInputConnection(flipX.GetOutputPort())
imageAppend.AddInputConnection(flipY.GetOutputPort())
imageAppend.SetAppendAxis(0)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(imageAppend.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#make interface
viewer.Render()
# --- end of script --
