#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

math = svtk.svtkMath()

# Generate some random colors
def MakeColors (lut, n):
    lut.SetNumberOfColors(n)
    lut.SetTableRange(0, n - 1)
    lut.SetScaleToLinear()
    lut.Build()
    lut.SetTableValue(0, 0, 0, 0, 1)
    math.RandomSeed(5071)
    i = 1
    while i < n:
        lut.SetTableValue(i, math.Random(.2, 1),
          math.Random(.2, 1), math.Random(.2, 1), 1)
        i += 1

lut = svtk.svtkLookupTable()
MakeColors(lut, 256)
n = 20
radius = 10

# This has been moved outside the loop so that the code can be correctly
# translated to python
blobImage = svtk.svtkImageData()

i = 0
while i < n:
    sphere = svtk.svtkSphere()
    sphere.SetRadius(radius)
    max = 50 - radius
    sphere.SetCenter(int(math.Random(-max, max)),
      int(math.Random(-max, max)), int(math.Random(-max, max)))

    sampler = svtk.svtkSampleFunction()
    sampler.SetImplicitFunction(sphere)
    sampler.SetOutputScalarTypeToFloat()
    sampler.SetSampleDimensions(51, 51, 51)
    sampler.SetModelBounds(-50, 50, -50, 50, -50, 50)

    thres = svtk.svtkImageThreshold()
    thres.SetInputConnection(sampler.GetOutputPort())
    thres.ThresholdByLower(radius * radius)
    thres.ReplaceInOn()
    thres.ReplaceOutOn()
    thres.SetInValue(i + 1)
    thres.SetOutValue(0)
    thres.Update()
    if (i == 0):
        blobImage.DeepCopy(thres.GetOutput())

    maxValue = svtk.svtkImageMathematics()
    maxValue.SetInputData(0, blobImage)
    maxValue.SetInputData(1, thres.GetOutput())
    maxValue.SetOperationToMax()
    maxValue.Modified()
    maxValue.Update()

    blobImage.DeepCopy(maxValue.GetOutput())

    i += 1

discrete = svtk.svtkDiscreteMarchingCubes()
discrete.SetInputData(blobImage)
discrete.GenerateValues(n, 1, n)
discrete.ComputeAdjacentScalarsOn() # creates PointScalars

thr = svtk.svtkThreshold()
thr.SetInputConnection(discrete.GetOutputPort())
thr.SetInputArrayToProcess(0, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, svtk.svtkDataSetAttributes.SCALARS) # act on PointScalars created by ComputeAdjacentScalarsOn
thr.AllScalarsOn() # default, changes better visible
thr.ThresholdBetween(0, 0) # remove cells between labels, i.e. keep cells neighbouring background (label 0)

vtu2vtp = svtk.svtkGeometryFilter()
vtu2vtp.SetInputConnection(thr.GetOutputPort())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(vtu2vtp.GetOutputPort())
mapper.SetLookupTable(lut)
mapper.SetScalarModeToUseCellData() # default is to use PointScalars, which get created with ComputeAdjacentScalarsOn
mapper.SetScalarRange(0, lut.GetNumberOfColors())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

ren1.AddActor(actor)

renWin.Render()

#iren.Start()
