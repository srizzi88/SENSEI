#!/usr/bin/env python
try:
    import numpy as np
except ImportError:
    print("Numpy (http://numpy.scipy.org) not found.")
    print("This test requires numpy!")
    from svtk.test import Testing
    Testing.skip()

import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
from svtk.numpy_interface import dataset_adapter as dsa
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
reader.SetFileName(SVTK_DATA_ROOT + "/Data/SPH_Points.vtu")
reader.Update()
output = reader.GetOutput()
scalarRange = output.GetPointData().GetArray("Rho").GetRange()

output2 = dsa.WrapDataObject(output)
# the constant value in the Cutoff array below is equal to
# the spatialStep (0.1) set below, and the CutoffFactor (3) of the QuinticKernel
Cutoff = np.ones(output.GetNumberOfPoints()) * 3.0/10.0;
Mass = np.ones(output.GetNumberOfPoints()) * 1.0;
output2.PointData.append(Mass, "Mass")
output2.PointData.append(Cutoff, "Cutoff")

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
plane.Push(1.15)
plane.Update()

# SPH kernel------------------------------------------------

sphKernel = svtk.svtkSPHQuinticKernel()
sphKernel.SetSpatialStep(0.1)

interpolator = svtk.svtkSPHInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceConnection(reader.GetOutputPort())
interpolator.SetDensityArrayName("Rho")
interpolator.SetMassArrayName("Mass")
interpolator.SetCutoffArrayName("Cutoff")
interpolator.SetKernel(sphKernel)

# Time execution
timer = svtk.svtkTimerLog()
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (SPH): {0}".format(time))
intMapper = svtk.svtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Rho")
intMapper.SetScalarRange(interpolator.GetOutput().GetPointData().GetArray("Rho").GetRange())

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
