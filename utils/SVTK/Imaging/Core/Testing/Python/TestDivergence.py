#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Divergence measures rate of change of gradient.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
gradient = svtk.svtkImageGradient()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
derivative = svtk.svtkImageDivergence()
derivative.SetInputConnection(gradient.GetOutputPort())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(derivative.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
