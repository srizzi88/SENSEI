#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# A script to test the mask filter.
#  replaces a circle with a color
# Image pipeline
reader = svtk.svtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/earth.ppm")
reader.Update()
sphere = svtk.svtkImageEllipsoidSource()
sphere.SetWholeExtent(0,511,0,255,0,0)
sphere.SetCenter(128,128,0)
sphere.SetRadius(80,80,1)
sphere.Update()
mask = svtk.svtkImageMask()
mask.SetImageInputData(reader.GetOutput())
mask.SetMaskInputData(sphere.GetOutput())
mask.SetMaskedOutputValue(100,128,200)
mask.NotMaskOn()
mask.ReleaseDataFlagOff()
mask.Update()
sphere2 = svtk.svtkImageEllipsoidSource()
sphere2.SetWholeExtent(0,511,0,255,0,0)
sphere2.SetCenter(328,128,0)
sphere2.SetRadius(80,50,1)
sphere2.Update()
# Test the wrapping of the output masked value
mask2 = svtk.svtkImageMask()
mask2.SetImageInputData(mask.GetOutput())
mask2.SetMaskInputData(sphere2.GetOutput())
mask2.SetMaskedOutputValue(100)
mask2.NotMaskOn()
mask2.ReleaseDataFlagOff()
mask2.Update()
sphere3 = svtk.svtkImageEllipsoidSource()
sphere3.SetWholeExtent(0,511,0,255,0,0)
sphere3.SetCenter(228,155,0)
sphere3.SetRadius(80,80,1)
sphere3.Update()
# Test the wrapping of the output masked value
mask3 = svtk.svtkImageMask()
mask3.SetImageInputData(mask2.GetOutput())
mask3.SetMaskInputData(sphere3.GetOutput())
mask3.SetMaskedOutputValue(255)
mask3.NotMaskOn()
mask3.SetMaskAlpha(0.5)
mask3.ReleaseDataFlagOff()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(mask3.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(128)
#viewer DebugOn
viewer.Render()
# --- end of script --
