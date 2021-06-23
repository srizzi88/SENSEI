#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

svtk.svtkMultiThreader.SetGlobalMaximumNumberOfThreads(1)

reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
rangeStart = 0.0
rangeEnd = 0.2
LUT = svtk.svtkLookupTable()
LUT.SetTableRange(0,1800)
LUT.SetSaturationRange(1,1)
LUT.SetHueRange(rangeStart,rangeEnd)
LUT.SetValueRange(1,1)
LUT.SetAlphaRange(1,1)
LUT.Build()
# The prototype of this function has
# arguments so that the code can be
# translated to python for testing.
def changeLUT (a=0,b=0,__svtk__temp0=0,__svtk__temp1=0):
    global rangeStart, rangeEnd
    rangeStart = expr.expr(globals(), locals(),["rangeStart","+","0.1"])
    rangeEnd = expr.expr(globals(), locals(),["rangeEnd","+","0.1"])
    if (rangeEnd > 1.0):
        rangeStart = 0.0
        rangeEnd = 0.2
        pass
    LUT.SetHueRange(rangeStart,rangeEnd)
    LUT.Build()

mapToRGBA = svtk.svtkImageMapToColors()
mapToRGBA.SetInputConnection(reader.GetOutputPort())
mapToRGBA.SetOutputFormatToRGBA()
mapToRGBA.SetLookupTable(LUT)
#mapToRGBA.AddObserver("EndEvent",changeLUT)
imageStreamer = svtk.svtkImageDataStreamer()
imageStreamer.SetInputConnection(mapToRGBA.GetOutputPort())
imageStreamer.SetNumberOfStreamDivisions(8)
# make sure we get the correct translator.
imageStreamer.UpdateInformation()
imageStreamer.GetExtentTranslator().SetSplitModeToBlock()
# set the window/level to 255.0/127.5 to view full range
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(imageStreamer.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(50)
viewer.Render()
#make interface
viewer.Render()
# --- end of script --
