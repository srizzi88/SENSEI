#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Image pipeline
createReader = svtk.svtkImageReader2Factory()
reader = createReader.CreateImageReader2("" + str(SVTK_DATA_ROOT) + "/Data/beach.jpg")
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/beach.jpg")
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
