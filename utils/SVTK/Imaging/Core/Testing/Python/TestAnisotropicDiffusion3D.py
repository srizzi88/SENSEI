#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Diffuses to 26 neighbors if difference is below threshold.
# Image pipeline
reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1,1,2)
diffusion = svtk.svtkImageAnisotropicDiffusion3D()
diffusion.SetInputConnection(reader.GetOutputPort())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(100.0)
diffusion.SetNumberOfIterations(5)
diffusion.GetExecutive().SetReleaseDataFlag(0,0)
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(diffusion.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)
#make interface
viewer.Render()
# --- end of script --
