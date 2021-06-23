#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.
# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
thresh = svtk.svtkImageThreshold()
thresh.SetInputConnection(reader.GetOutputPort())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()
island = svtk.svtkImageIslandRemoval2D()
island.SetInputConnection(thresh.GetOutputPort())
island.SetIslandValue(255)
island.SetReplaceValue(128)
island.SetAreaThreshold(100)
island.SquareNeighborhoodOn()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(island.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer DebugOn
viewer.Render()
# --- end of script --
