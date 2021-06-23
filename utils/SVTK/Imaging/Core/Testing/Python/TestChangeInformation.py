#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# In this example, an image is centered at (0,0,0) before a
# rotation is applied to ensure that the rotation occurs about
# the center of the image.
reader = svtk.svtkPNGReader()
reader.SetDataSpacing(0.8,0.8,1.5)
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
# first center the image at (0,0,0)
information = svtk.svtkImageChangeInformation()
information.SetInputConnection(reader.GetOutputPort())
information.CenterImageOn()
reslice = svtk.svtkImageReslice()
reslice.SetInputConnection(information.GetOutputPort())
reslice.SetResliceAxesDirectionCosines([0.866025,-0.5,0,0.5,0.866025,0,0,0,1])
reslice.SetInterpolationModeToCubic()
# reset the image back to the way it was (you don't have
# to do this, it is just put in as an example)
information2 = svtk.svtkImageChangeInformation()
information2.SetInputConnection(reslice.GetOutputPort())
reader.Update()
information2.SetInformationInputData(reader.GetOutput())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(information2.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
