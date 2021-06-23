#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
clip = svtk.svtkImageClip()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetOutputWholeExtent(80,230,80,230,0,0)
clip.ClipDataOff()
gradient = svtk.svtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(clip.GetOutputPort())
gradient.HandleBoundariesOff()
slide = svtk.svtkImageChangeInformation()
slide.SetInputConnection(gradient.GetOutputPort())
slide.SetExtentTranslation(-100,-100,0)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(slide.GetOutputPort())
viewer.SetColorWindow(-1000)
viewer.SetColorLevel(500)
viewer.SetSize(150,150)
viewer.Render()
#skipping source
# --- end of script --
