#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# this test is designed to check the operation of the 8bit
# export of BMPs
# Image pipeline
reader = svtk.svtkBMPReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
reader.SetAllow8BitBMP(1)
map = svtk.svtkImageMapToColors()
map.SetInputConnection(reader.GetOutputPort())
map.SetLookupTable(reader.GetLookupTable())
map.SetOutputFormatToRGB()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(map.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
