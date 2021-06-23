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

# create pipeline
#
reader = svtk.svtkXMLUnstructuredGridReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/SPH_Points2D.vtu")
reader.Update()
output = reader.GetOutput()
scalarRange = output.GetPointData().GetArray("Rho").GetRange()
print("Scalar range: {}".format(scalarRange))

# Something to sample with
center = output.GetCenter()
bounds = output.GetBounds()
length = output.GetLength()

plane = svtk.svtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(bounds[0],bounds[2],bounds[4])
plane.SetPoint1(bounds[1],bounds[2],bounds[4])
plane.SetPoint2(bounds[0],bounds[3],bounds[4])
plane.SetCenter(center)
plane.SetNormal(0,0,1)
plane.Update()

timer = svtk.svtkTimerLog()

# SPH kernel------------------------------------------------
sphKernel = svtk.svtkSPHQuinticKernel()
sphKernel.SetSpatialStep(0.00002)
sphKernel.SetDimension(2)

interpolator = svtk.svtkSPHInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceConnection(reader.GetOutputPort())
interpolator.SetKernel(sphKernel)
interpolator.Update()

# Time execution
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (SPH): {0}".format(time))

intMapper = svtk.svtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Rho")
intMapper.SetScalarRange(750,1050)

intActor = svtk.svtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = svtk.svtkOutlineFilter()
outline.SetInputData(output)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

iren.Initialize()
renWin.Render()

iren.Start()
