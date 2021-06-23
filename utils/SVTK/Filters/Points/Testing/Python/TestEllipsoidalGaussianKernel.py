#!/usr/bin/env python
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Parameters for testing
res = 250

# Graphics stuff
ren0 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren0)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a volume to interpolate on to
volume = svtk.svtkImageData()
volume.SetDimensions(res,res,res)
volume.SetOrigin(0,0,0)
volume.SetSpacing(1,1,1)
fa = svtk.svtkFloatArray()
fa.SetName("scalars")
fa.Allocate(res ** 3)
volume.GetPointData().SetScalars(fa)

center = volume.GetCenter()
bounds = volume.GetBounds()

# Create a single point with a normal and scalar
onePts = svtk.svtkPoints()
onePts.SetNumberOfPoints(1)
onePts.SetPoint(0,center)

oneScalars = svtk.svtkFloatArray()
oneScalars.SetNumberOfTuples(1)
oneScalars.SetTuple1(0,5.0)
oneScalars.SetName("scalarPt")

oneNormals = svtk.svtkFloatArray()
oneNormals.SetNumberOfComponents(3)
oneNormals.SetNumberOfTuples(1)
oneNormals.SetTuple3(0,1,1,1)
oneNormals.SetName("normalPt")

oneData = svtk.svtkPolyData()
oneData.SetPoints(onePts)
oneData.GetPointData().SetScalars(oneScalars)
oneData.GetPointData().SetNormals(oneNormals)

#  Interpolation ------------------------------------------------
eKernel = svtk.svtkEllipsoidalGaussianKernel()
eKernel.SetKernelFootprintToRadius()
eKernel.SetRadius(50.0)
eKernel.UseScalarsOn()
eKernel.UseNormalsOn()
eKernel.SetScaleFactor(0.5)
eKernel.SetEccentricity(3)
eKernel.NormalizeWeightsOff()

interpolator = svtk.svtkPointInterpolator()
interpolator.SetInputData(volume)
interpolator.SetSourceData(oneData)
interpolator.SetKernel(eKernel)
interpolator.Update()

#  Extract iso surface ------------------------------------------------
contour = svtk.svtkFlyingEdges3D()
contour.SetInputConnection(interpolator.GetOutputPort())
contour.SetValue(0,10)

intMapper = svtk.svtkPolyDataMapper()
intMapper.SetInputConnection(contour.GetOutputPort())

intActor = svtk.svtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputData(volume)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
#ren0.AddActor(outlineActor)
ren0.SetBackground(1,1,1)

iren.Initialize()
renWin.Render()

iren.Start()
