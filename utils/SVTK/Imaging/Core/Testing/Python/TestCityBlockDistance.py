#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.
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
dist = svtk.svtkImageCityBlockDistance()
dist.SetDimensionality(2)
dist.SetInputConnection(thresh.GetOutputPort())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(dist.GetOutputPort())
viewer.SetColorWindow(117)
viewer.SetColorLevel(43)
viewer.Render()
# --- end of script --
