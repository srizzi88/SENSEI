#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This script shows how to use svtkImageEuclideanDistance
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = svtk.svtkImageCast()
cast.SetOutputScalarTypeToShort()
cast.SetInputConnection(reader.GetOutputPort())
thresh = svtk.svtkImageThreshold()
thresh.SetInputConnection(cast.GetOutputPort())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(0)
thresh.SetOutValue(200)
thresh.ReleaseDataFlagOff()
dist = svtk.svtkImageEuclideanDistance()
dist.SetInputConnection(thresh.GetOutputPort())
dist.SetAlgorithmToSaitoCached()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(dist.GetOutputPort())
viewer.SetColorWindow(117)
viewer.SetColorLevel(43)
viewer.Render()
#skipping source
# --- end of script --
