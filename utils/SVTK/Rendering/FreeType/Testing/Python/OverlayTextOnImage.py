#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

#
# display text over an image
#
ellipse = svtk.svtkImageEllipsoidSource()
mapImage = svtk.svtkImageMapper()
mapImage.SetInputConnection(ellipse.GetOutputPort())
mapImage.SetColorWindow(255)
mapImage.SetColorLevel(127.5)
img = svtk.svtkActor2D()
img.SetMapper(mapImage)
mapText = svtk.svtkTextMapper()
mapText.SetInput("Text Overlay")
mapText.GetTextProperty().SetFontSize(15)
mapText.GetTextProperty().SetColor(0,1,1)
mapText.GetTextProperty().BoldOn()
mapText.GetTextProperty().ShadowOn()
txt = svtk.svtkActor2D()
txt.SetMapper(mapText)
txt.SetPosition(138,128)
ren1 = svtk.svtkRenderer()
ren1.AddActor2D(img)
ren1.AddActor2D(txt)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
iren.Initialize()
# --- end of script --
