#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This Script test the euclidean to polar by converting 2D vectors
# from a gradient into polar, which is converted into HSV, and then to RGB.
# Image pipeline
gauss = svtk.svtkImageGaussianSource()
gauss.SetWholeExtent(0,255,0,255,0,44)
gauss.SetCenter(127,127,22)
gauss.SetStandardDeviation(50.0)
gauss.SetMaximum(10000.0)
gradient = svtk.svtkImageGradient()
gradient.SetInputConnection(gauss.GetOutputPort())
gradient.SetDimensionality(2)
polar = svtk.svtkImageEuclideanToPolar()
polar.SetInputConnection(gradient.GetOutputPort())
polar.SetThetaMaximum(255)
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(polar.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
