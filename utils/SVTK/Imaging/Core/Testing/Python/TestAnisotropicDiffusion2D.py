#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
reader = svtk.svtkPNGReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/fullhead15.png")
diffusion = svtk.svtkImageAnisotropicDiffusion2D()
diffusion.SetInputConnection(reader.GetOutputPort())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(200.0)
diffusion.SetNumberOfIterations(5)
#diffusion DebugOn
viewer = svtk.svtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(diffusion.GetOutputPort())
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)
viewer.Render()
# --- end of script --
