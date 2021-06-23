#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Set up the pipeline
reader = svtk.svtkTIFFReader()
reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a svtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
reader.SetOrientationType(4)
ia = svtk.svtkImageActor()
ia.GetMapper().SetInputConnection(reader.GetOutputPort())
ren = svtk.svtkRenderer()
ren.AddActor(ia)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
rbz = svtk.svtkInteractorStyleRubberBandZoom()
rbz.SetInteractor(iren)
iren.SetInteractorStyle(rbz)
renWin.Render()
ren.GetActiveCamera().SetClippingRange(538.2413295991446, 551.8332823667997)
# Test style
iren.SetEventInformationFlipY(250,250,0,0,"0",0,"0")
iren.InvokeEvent("LeftButtonPressEvent")
iren.SetEventInformationFlipY(100,100,0,0,"0",0,"0")
iren.InvokeEvent("MouseMoveEvent")
iren.InvokeEvent("LeftButtonReleaseEvent")
# --- end of script --
