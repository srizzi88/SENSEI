#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.
# Image pipeline
reader = svtk.svtkDICOMImageReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/mr.001")
sobel = svtk.svtkImageSobel2D()
sobel.SetInputConnection(reader.GetOutputPort())
sobel.ReleaseDataFlagOff()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(sobel.GetOutputPort())
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
