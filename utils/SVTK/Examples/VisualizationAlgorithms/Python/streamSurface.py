#!/usr/bin/env python

# This example demonstrates the generation of a streamsurface.

import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read the data and specify which scalars and vectors to read.
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(SVTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(SVTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# We use a rake to generate a series of streamline starting points
# scattered along a line. Each point will generate a streamline. These
# streamlines are then fed to the svtkRuledSurfaceFilter which stitches
# the lines together to form a surface.
rake = svtk.svtkLineSource()
rake.SetPoint1(15, -5, 32)
rake.SetPoint2(15, 5, 32)
rake.SetResolution(21)
rakeMapper = svtk.svtkPolyDataMapper()
rakeMapper.SetInputConnection(rake.GetOutputPort())
rakeActor = svtk.svtkActor()
rakeActor.SetMapper(rakeMapper)

integ = svtk.svtkRungeKutta4()
sl = svtk.svtkStreamTracer()
sl.SetInputData(pl3d_output)
sl.SetSourceConnection(rake.GetOutputPort())
sl.SetIntegrator(integ)
sl.SetMaximumPropagation(100)
sl.SetInitialIntegrationStep(0.1)
sl.SetIntegrationDirectionToBackward()

# The ruled surface stiches together lines with triangle strips.
# Note the SetOnRatio method. It turns on every other strip that
# the filter generates (only when multiple lines are input).
scalarSurface = svtk.svtkRuledSurfaceFilter()
scalarSurface.SetInputConnection(sl.GetOutputPort())
scalarSurface.SetOffset(0)
scalarSurface.SetOnRatio(2)
scalarSurface.PassLinesOn()
scalarSurface.SetRuledModeToPointWalk()
scalarSurface.SetDistanceFactor(30)
mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(scalarSurface.GetOutputPort())
mapper.SetScalarRange(pl3d_output.GetScalarRange())
actor = svtk.svtkActor()
actor.SetMapper(mapper)

# Put an outline around for context.
outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Now create the usual graphics stuff.
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(rakeActor)
ren.AddActor(actor)
ren.AddActor(outlineActor)
ren.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()
iren.Start()
