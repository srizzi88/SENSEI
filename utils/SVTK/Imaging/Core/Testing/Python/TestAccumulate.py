#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
smooth = svtk.svtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(1,1)
smooth.SetInputConnection(reader.GetOutputPort())
imageAppend = svtk.svtkImageAppendComponents()
imageAppend.AddInputConnection(reader.GetOutputPort())
imageAppend.AddInputConnection(smooth.GetOutputPort())
clip = svtk.svtkImageClip()
clip.SetInputConnection(imageAppend.GetOutputPort())
clip.SetOutputWholeExtent(0,255,0,255,20,22)
accum = svtk.svtkImageAccumulate()
accum.SetInputConnection(clip.GetOutputPort())
accum.SetComponentExtent(0,255,0,255,0,0)
accum.SetComponentSpacing(12,12,0.0)
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(accum.GetOutputPort())
viewer.SetColorWindow(4)
viewer.SetColorLevel(2)
viewer.Render()
# --- end of script --
