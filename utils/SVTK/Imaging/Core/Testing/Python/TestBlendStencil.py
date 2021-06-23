#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# do alpha-blending of two images, but also clip with stencil
reader1 = svtk.svtkBMPReader()
reader1.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")

reader2 = svtk.svtkPNMReader()
reader2.SetFileName(SVTK_DATA_ROOT + "/Data/B.pgm")

table = svtk.svtkLookupTable()
table.SetTableRange(0, 127)
table.SetValueRange(0.0, 1.0)
table.SetSaturationRange(0.0, 0.0)
table.SetHueRange(0.0, 0.0)
table.SetAlphaRange(0.9, 0.0)
table.Build()

translate = svtk.svtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60, 60, 0)

sphere = svtk.svtkSphere()
sphere.SetCenter(121, 131, 0)
sphere.SetRadius(70)

functionToStencil = svtk.svtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)

blend = svtk.svtkImageBlend()
blend.SetInputConnection(reader1.GetOutputPort())
blend.AddInputConnection(translate.GetOutputPort())

# exercise the ReplaceNthInputConnection method
blend.ReplaceNthInputConnection(1, reader1.GetOutputPort())
blend.ReplaceNthInputConnection(1, translate.GetOutputPort())
blend.SetOpacity(1, 0.8)
blend.SetStencilConnection(functionToStencil.GetOutputPort())

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(blend.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
