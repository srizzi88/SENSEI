#!/usr/bin/env python

# This example demonstrates how to extract "computational planes" from
# a structured dataset. Structured data has a natural, logical
# coordinate system based on i-j-k indices. Specifying imin,imax,
# jmin,jmax, kmin,kmax pairs can indicate a point, line, plane, or
# volume of data.
#
# In this example, we extract three planes and warp them using scalar
# values in the direction of the local normal at each point. This
# gives a sort of "velocity profile" that indicates the nature of the
# flow.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Here we read data from a annular combustor. A combustor burns fuel
# and air in a gas turbine (e.g., a jet engine) and the hot gas
# eventually makes its way to the turbine section.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# Planes are specified using a imin,imax, jmin,jmax, kmin,kmax
# coordinate specification. Min and max i,j,k values are clamped to 0
# and maximum value.
plane = svtk.svtkStructuredGridGeometryFilter()
plane.SetInputData(pl3d_output)
plane.SetExtent(10, 10, 1, 100, 1, 100)
plane2 = svtk.svtkStructuredGridGeometryFilter()
plane2.SetInputData(pl3d_output)
plane2.SetExtent(30, 30, 1, 100, 1, 100)
plane3 = svtk.svtkStructuredGridGeometryFilter()
plane3.SetInputData(pl3d_output)
plane3.SetExtent(45, 45, 1, 100, 1, 100)

# We use an append filter because that way we can do the warping,
# etc. just using a single pipeline and actor.
appendF = svtk.svtkAppendPolyData()
appendF.AddInputConnection(plane.GetOutputPort())
appendF.AddInputConnection(plane2.GetOutputPort())
appendF.AddInputConnection(plane3.GetOutputPort())
warp = svtk.svtkWarpScalar()
warp.SetInputConnection(appendF.GetOutputPort())
warp.UseNormalOn()
warp.SetNormal(1.0, 0.0, 0.0)
warp.SetScaleFactor(2.5)
normals = svtk.svtkPolyDataNormals()
normals.SetInputConnection(warp.GetOutputPort())
normals.SetFeatureAngle(60)
planeMapper = svtk.svtkPolyDataMapper()
planeMapper.SetInputConnection(normals.GetOutputPort())
planeMapper.SetScalarRange(pl3d_output.GetScalarRange())
planeActor = svtk.svtkActor()
planeActor.SetMapper(planeMapper)

# The outline provides context for the data and the planes.
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the usual graphics stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)

# Create an initial view.
cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam1.SetPosition(-12.3332, 31.7479, 41.2387)
cam1.SetViewUp(0.060772, -0.319905, 0.945498)

iren.Initialize()
renWin.Render()
iren.Start()
