#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkMINCImageReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/t3_grid_0.mnc")

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(65535)
viewer.SetColorLevel(0)

# make interface
viewer.Render()
