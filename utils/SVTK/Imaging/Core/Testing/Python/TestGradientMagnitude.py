#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkGESignaReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/E07733S002I009.MR")
gradient = svtk.svtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(gradient.GetOutputPort())
viewer.SetColorWindow(250)
viewer.SetColorLevel(125)
viewer.Render()
# --- end of script --
