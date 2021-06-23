#!/usr/bin/env python

# This simple example shows how to render a very large image (i.e.
# one that cannot fit on the screen).

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# We'll import some data to start. Since we are using an importer,
# we've got to give it a render window and such. Note that the render
# window size is set fairly small.
ren = svtk.svtkRenderer()
ren.SetBackground(0.1, 0.2, 0.4)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(125, 125)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

importer = svtk.svtk3DSImporter()
importer.SetRenderWindow(renWin)
importer.SetFileName(SVTK_DATA_ROOT + "/Data/Viewpoint/iflamigm.3ds")
importer.ComputeNormalsOn()
importer.Read()

# We'll set up the view we want.
ren.GetActiveCamera().SetPosition(0, 1, 0)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 0, 1)

# Let the renderer compute a good position and focal point.
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.4)
ren.ResetCameraClippingRange()

renderLarge = svtk.svtkRenderLargeImage()
renderLarge.SetInput(ren)
renderLarge.SetMagnification(4)

# We write out the image which causes the rendering to occur. If you
# watch your screen you might see the pieces being rendered right
# after one another.
writer = svtk.svtkPNGWriter()
writer.SetInputConnection(renderLarge.GetOutputPort())
writer.SetFileName("largeImage.png")
writer.Write()
