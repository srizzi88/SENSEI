#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Example demonstrates how to generate a 3D tetrahedra mesh from a volume. This example
# differs from clipVolume.tcl in that the mesh is generated within a range of contour values.
#
# Quadric definition
quadric = svtk.svtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = svtk.svtkSampleFunction()
sample.SetSampleDimensions(20,20,20)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
# Program a bandpass filter to clip a range of data. What we do is transform the
# scalars so that values laying between (minRange,maxRange) are >= 0.0; all
# others are < 0.0,
dataset = svtk.svtkImplicitDataSet()
dataset.SetDataSet(sample.GetOutput())
window = svtk.svtkImplicitWindowFunction()
window.SetImplicitFunction(dataset)
window.SetWindowRange(0.5,1.0)
# Generate tetrahedral mesh
clip = svtk.svtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetClipFunction(window)
clip.SetValue(0.0)
clip.GenerateClippedOutputOff()
clipMapper = svtk.svtkDataSetMapper()
clipMapper.SetInputConnection(clip.GetOutputPort())
clipMapper.ScalarVisibilityOff()
clipActor = svtk.svtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(.8,.4,.4)
# Create outline
outline = svtk.svtkOutlineFilter()
#  outline SetInputData [clip GetInput]
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Define graphics objects
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(clipActor)
ren1.AddActor(outlineActor)
iren.Initialize()
# --- end of script --
