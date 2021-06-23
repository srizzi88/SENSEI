#!/usr/bin/env python

# This example demonstrates how to use a programmable filter and how
# to use the special svtkDataSetToDataSet::GetOutputPort() methods

import svtk
from math import *

# We create a 100 by 100 point plane to sample
plane = svtk.svtkPlaneSource()
plane.SetXResolution(100)
plane.SetYResolution(100)

# We transform the plane by a factor of 10 on X and Y
transform = svtk.svtkTransform()
transform.Scale(10, 10, 1)
transF = svtk.svtkTransformPolyDataFilter()
transF.SetInputConnection(plane.GetOutputPort())
transF.SetTransform(transform)

# Compute Bessel function and derivatives. We'll use a programmable filter
# for this. Note the unusual GetPolyDataInput() & GetOutputPort() methods.
besselF = svtk.svtkProgrammableFilter()
besselF.SetInputConnection(transF.GetOutputPort())

# The SetExecuteMethod takes a Python function as an argument
# In here is where all the processing is done.
def bessel():
    input = besselF.GetPolyDataInput()
    numPts = input.GetNumberOfPoints()
    newPts = svtk.svtkPoints()
    derivs = svtk.svtkFloatArray()

    for i in range(0, numPts):
        x = input.GetPoint(i)
        x0, x1 = x[:2]

        r = sqrt(x0*x0+x1*x1)
        x2 = exp(-r)*cos(10.0*r)
        deriv = -exp(-r)*(cos(10.0*r)+10.0*sin(10.0*r))

        newPts.InsertPoint(i, x0, x1, x2)
        derivs.InsertValue(i, deriv)

    besselF.GetPolyDataOutput().CopyStructure(input)
    besselF.GetPolyDataOutput().SetPoints(newPts)
    besselF.GetPolyDataOutput().GetPointData().SetScalars(derivs)

besselF.SetExecuteMethod(bessel)

# We warp the plane based on the scalar values calculated above
warp = svtk.svtkWarpScalar()
warp.SetInputConnection(besselF.GetOutputPort())
warp.XYPlaneOn()
warp.SetScaleFactor(0.5)


# We create a mapper and actor as usual. In the case we adjust the
# scalar range of the mapper to match that of the computed scalars
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(warp.GetOutputPort())
mapper.SetScalarRange(besselF.GetPolyDataOutput().GetScalarRange())
carpet = svtk.svtkActor()
carpet.SetMapper(mapper)

# Create the RenderWindow, Renderer
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(carpet)
renWin.SetSize(500, 500)

ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
