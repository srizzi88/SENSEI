#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
imageIn = svtk.svtkPNMReader()
imageIn.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/B.pgm")
gaussian = svtk.svtkImageGaussianSmooth()
gaussian.SetStandardDeviations(2,2)
gaussian.SetDimensionality(2)
gaussian.SetRadiusFactors(1,1)
gaussian.SetInputConnection(imageIn.GetOutputPort())
geometry = svtk.svtkImageDataGeometryFilter()
geometry.SetInputConnection(gaussian.GetOutputPort())
aClipper = svtk.svtkClipPolyData()
aClipper.SetInputConnection(geometry.GetOutputPort())
aClipper.SetValue(127.5)
aClipper.GenerateClipScalarsOff()
aClipper.InsideOutOn()
aClipper.GetOutput().GetPointData().CopyScalarsOff()
aClipper.Update()
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(aClipper.GetOutputPort())
mapper.ScalarVisibilityOff()
letter = svtk.svtkActor()
letter.SetMapper(mapper)
ren1.AddActor(letter)
letter.GetProperty().SetDiffuseColor(0,0,0)
letter.GetProperty().SetRepresentationToWireframe()
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.SetSize(320,320)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
