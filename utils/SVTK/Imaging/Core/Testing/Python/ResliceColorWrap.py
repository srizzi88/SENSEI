#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# we have to make sure it works with multiple scalar components
# Image pipeline
reader = svtk.svtkBMPReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/masonry.bmp")
reader.SetDataExtent(0,255,0,255,0,0)
reader.SetDataSpacing(1,1,1)
reader.SetDataOrigin(0,0,0)
reader.UpdateWholeExtent()
transform = svtk.svtkTransform()
transform.RotateZ(45)
transform.Scale(1.414,1.414,1.414)
reslice = svtk.svtkImageReslice()
reslice.SetInputConnection(reader.GetOutputPort())
reslice.SetResliceTransform(transform)
reslice.InterpolateOn()
reslice.SetInterpolationModeToCubic()
reslice.WrapOn()
reslice.AutoCropOutputOn()
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(reslice.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(256.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
