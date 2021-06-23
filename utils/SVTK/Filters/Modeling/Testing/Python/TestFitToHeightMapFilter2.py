#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Debugging control
iDim = 5
jDim = 7
imgSize = iDim * jDim

# Create the RenderWindow, Renderer
#
ren0 = svtk.svtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = svtk.svtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer( ren0 )
renWin.AddRenderer( ren1 )

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline.
#
lut = svtk.svtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Create an image with random heights on each pixel
image = svtk.svtkImageData()
image.SetDimensions(iDim,jDim,1)
image.SetSpacing(1,1,1)
image.SetOrigin(0,0,0)

math = svtk.svtkMath()
math.RandomSeed(31415)
heights = svtk.svtkFloatArray()
heights.SetNumberOfTuples(imgSize)
for i in range(0,imgSize):
    heights.SetTuple1(i,math.Random(0,1))

image.GetPointData().SetScalars(heights)

# Warp the image data
lo = image.GetScalarRange()[0]
hi = image.GetScalarRange()[1]

surface = svtk.svtkImageDataGeometryFilter()
surface.SetInputData(image)

tris = svtk.svtkTriangleFilter()
tris.SetInputConnection(surface.GetOutputPort())

warp = svtk.svtkWarpScalar()
warp.SetInputConnection(tris.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)

# Show the terrain
imgMapper = svtk.svtkPolyDataMapper()
imgMapper.SetInputConnection(warp.GetOutputPort())
imgMapper.SetScalarRange(lo, hi)
imgMapper.SetLookupTable(lut)

imgActor = svtk.svtkActor()
imgActor.SetMapper(imgMapper)

# Polydata to drape
plane = svtk.svtkPlaneSource()
plane.SetOrigin(-0.1,-0.1,0)
plane.SetPoint1(iDim-1+0.1,-0.1,0)
plane.SetPoint2(-0.1,jDim-1+0.1,0)
plane.SetResolution(40,20)

# Fit polygons to surface (point strategy)
fit = svtk.svtkFitToHeightMapFilter()
fit.SetInputConnection(plane.GetOutputPort())
fit.SetHeightMapData(image)
fit.SetFittingStrategyToPointProjection()
fit.UseHeightMapOffsetOn()
fit.Update()
print(fit.GetOutput().GetBounds())

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(fit.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Fit polygons to surface (cell strategy)
fit2 = svtk.svtkFitToHeightMapFilter()
fit2.SetInputConnection(plane.GetOutputPort())
fit2.SetHeightMapData(image)
fit2.SetFittingStrategyToCellAverageHeight()
fit2.UseHeightMapOffsetOn()
fit2.Update()

mapper2 = svtk.svtkPolyDataMapper()
mapper2.SetInputConnection(fit2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = svtk.svtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetColor(1,0,0)

# Render it
ren0.AddActor(actor)
ren0.AddActor(imgActor)
ren1.AddActor(actor2)
ren1.AddActor(imgActor)

ren0.GetActiveCamera().SetPosition(1,1,1)
ren0.ResetCamera()
ren1.SetActiveCamera(ren0.GetActiveCamera())

renWin.Render()
iren.Start()
