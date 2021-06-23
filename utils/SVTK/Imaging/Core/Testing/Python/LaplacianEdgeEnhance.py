#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script subtracts the 2D laplacian from an image to enhance the edges.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = svtk.svtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToDouble()
cast.Update()
lap = svtk.svtkImageLaplacian()
lap.SetInputConnection(cast.GetOutputPort())
lap.SetDimensionality(2)
lap.Update()
subtract = svtk.svtkImageMathematics()
subtract.SetOperationToSubtract()
subtract.SetInput1Data(cast.GetOutput())
subtract.SetInput2Data(lap.GetOutput())
subtract.ReleaseDataFlagOff()
#subtract BypassOn
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(subtract.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
