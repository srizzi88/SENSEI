#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

ren1 = svtk.svtkRenderer()
renWin1 = svtk.svtkRenderWindow()
renWin1.AddRenderer(ren1)

importer = svtk.svtk3DSImporter()
importer.SetRenderWindow(renWin1)
importer.ComputeNormalsOn()
importer.SetFileName(SVTK_DATA_ROOT + "/Data/iflamigm.3ds")
importer.Read()

importer.GetRenderer().SetBackground(0.1, 0.2, 0.4)
importer.GetRenderWindow().SetSize(150, 150)

#
# the importer created the renderer
renCollection = renWin1.GetRenderers()
renCollection.InitTraversal()

ren = renCollection.GetNextItem()

#
# change view up to +z
#
ren.GetActiveCamera().SetWindowCenter(-0.2, 0.3)
ren.GetActiveCamera().SetPosition(0, 1, 0)
ren.GetActiveCamera().SetFocalPoint(0, 0, 0)
ren.GetActiveCamera().SetViewUp(0, 0, 1)

#
# let the renderer compute good position and focal point
#
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.4)
ren1.ResetCameraClippingRange()

# render the large image
#
renderLarge = svtk.svtkRenderLargeImage()
renderLarge.SetInput(ren1)
renderLarge.SetMagnification(3)
renderLarge.Update()

viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(renderLarge.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()

## on several opengl X window unix implementations
## multiple context deletes cause errors
## so we leak the renWin in this test for unix
#if renWin1.IsA('svtkXOpenGLRenderWindow'):
#    renWin1.Register(ren1)
#    dl = svtk.svtkDebugLeaks()
#    dl.SetExitError(0)
#    del dl

# iren.Initialize()
# iren.Start()
