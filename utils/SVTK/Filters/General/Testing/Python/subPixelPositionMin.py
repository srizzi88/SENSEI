#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Test sub pixel positioning (A round about way of getting an iso surface.)
# See cubed sphere for the surface before sub pixel poisitioning.
sphere = svtk.svtkSphere()
sphere.SetCenter(1,1,1)
sphere.SetRadius(0.9)
sample = svtk.svtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(0,2,0,2,0,2)
sample.SetSampleDimensions(30,30,30)
sample.ComputeNormalsOff()
sample.Update()
threshold1 = svtk.svtkThreshold()
threshold1.SetInputConnection(sample.GetOutputPort())
threshold1.ThresholdByLower(0.001)
geometry = svtk.svtkGeometryFilter()
geometry.SetInputConnection(threshold1.GetOutputPort())
grad = svtk.svtkImageGradient()
grad.SetDimensionality(3)
grad.SetInputConnection(sample.GetOutputPort())
grad.Update()
mult = svtk.svtkImageMathematics()
mult.SetOperationToMultiply()
mult.SetInput1Data(sample.GetOutput())
mult.SetInput2Data(sample.GetOutput())
itosp = svtk.svtkImageToStructuredPoints()
itosp.SetInputConnection(mult.GetOutputPort())
itosp.SetVectorInputData(grad.GetOutput())
itosp.Update()
sub = svtk.svtkSubPixelPositionEdgels()
sub.SetInputConnection(geometry.GetOutputPort())
sub.SetGradMapsData(itosp.GetOutput())
mapper = svtk.svtkDataSetMapper()
mapper.SetInputConnection(sub.GetOutputPort())
actor = svtk.svtkActor()
actor.SetMapper(mapper)
# Create renderer stuff
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(20)
ren1.GetActiveCamera().Elevation(30)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
