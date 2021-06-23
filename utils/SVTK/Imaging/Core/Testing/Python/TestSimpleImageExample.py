#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
gradient = svtk.svtkSimpleImageFilterExample()
gradient.SetInputConnection(reader.GetOutputPort())
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(gradient.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
viewer.Render()
# --- end of script --
