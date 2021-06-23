#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows the magnitude of an image in frequency domain.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = svtk.svtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToFloat()
scale2 = svtk.svtkImageShiftScale()
scale2.SetInputConnection(cast.GetOutputPort())
scale2.SetScale(0.05)
gradient = svtk.svtkImageGradient()
gradient.SetInputConnection(scale2.GetOutputPort())
gradient.SetDimensionality(3)
gradient.Update()
pnm = svtk.svtkBMPReader()
pnm.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
cast2 = svtk.svtkImageCast()
cast2.SetInputConnection(pnm.GetOutputPort())
cast2.SetOutputScalarTypeToDouble()
cast2.Update()
magnitude = svtk.svtkImageDotProduct()
magnitude.SetInput1Data(cast2.GetOutput())
magnitude.SetInput2Data(gradient.GetOutput())
#svtkImageViewer viewer
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(magnitude.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(300)
#viewer DebugOn
viewer.Render()
# --- end of script --
