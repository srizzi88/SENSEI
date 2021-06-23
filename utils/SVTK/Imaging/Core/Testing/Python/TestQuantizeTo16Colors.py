#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Make an image larger by repeating the data.  Tile.
# Image pipeline
reader = svtk.svtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/earth.ppm")
pad = svtk.svtkImageMirrorPad()
pad.SetInputConnection(reader.GetOutputPort())
pad.SetOutputWholeExtent(-120,320,-120,320,0,0)
quant = svtk.svtkImageQuantizeRGBToIndex()
quant.SetInputConnection(pad.GetOutputPort())
quant.SetNumberOfColors(16)
quant.Update()
map = svtk.svtkImageMapToRGBA()
map.SetInputConnection(quant.GetOutputPort())
map.SetLookupTable(quant.GetLookupTable())
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(map.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127)
viewer.GetActor2D().SetDisplayPosition(110,110)
viewer.SetSize(440,440)
viewer.Render()
# --- end of script --
