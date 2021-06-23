#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Tst the OpenClose3D filter.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
thresh = svtk.svtkImageThreshold()
thresh.SetInputConnection(reader.GetOutputPort())
thresh.SetOutputScalarTypeToUnsignedChar()
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()
my_close = svtk.svtkImageOpenClose3D()
my_close.SetInputConnection(thresh.GetOutputPort())
my_close.SetOpenValue(0)
my_close.SetCloseValue(255)
my_close.SetKernelSize(5,5,3)
my_close.ReleaseDataFlagOff()
# for coverage (we could compare results to see if they are correct).
my_close.DebugOn()
my_close.DebugOff()
my_close.GetOutput()
my_close.GetCloseValue()
my_close.GetOpenValue()
#my_close AddObserver ProgressEvent {set pro [my_close GetProgress]; puts "Completed $pro"; flush stdout}
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(my_close.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
