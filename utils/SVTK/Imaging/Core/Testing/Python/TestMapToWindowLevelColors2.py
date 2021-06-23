#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtk.svtkBMPReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
# set the window/level
viewer = svtk.svtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(100.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
