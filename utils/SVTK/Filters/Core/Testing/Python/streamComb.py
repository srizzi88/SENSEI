#!/usr/bin/env python

import svtk
from svtk.util.misc import svtkGetDataRoot

# create planes
# Create the RenderWindow, Renderer
#
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer( ren )

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = svtk.svtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName( svtkGetDataRoot() + '/Data/combxyz.bin' )
pl3d.SetQFileName( svtkGetDataRoot() + '/Data/combq.bin' )
pl3d.SetScalarFunctionNumber( 100 )
pl3d.SetVectorFunctionNumber( 202 )
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

outline = svtk.svtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

seeds = svtk.svtkLineSource()
seeds.SetPoint1(15, -5, 32)
seeds.SetPoint2(15, 5, 32)
seeds.SetResolution(10)

sl = svtk.svtkStreamTracer()
sl.SetInputData(pl3d_output)
sl.SetSourceConnection(seeds.GetOutputPort())
sl.SetMaximumPropagation(100)
sl.SetInitialIntegrationStep(0.1)
sl.SetIntegrationDirectionToBoth()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(sl.GetOutputPort())

actor = svtk.svtkActor()
actor.SetMapper(mapper)

mmapper = svtk.svtkPolyDataMapper()
mmapper.SetInputConnection(seeds.GetOutputPort())
mactor = svtk.svtkActor()
mactor.SetMapper(mmapper)
ren.AddActor(mactor)

ren.AddActor(actor)
ren.AddActor(outlineActor)

cam=ren.GetActiveCamera()
cam.SetClippingRange( 3.95297, 50 )
cam.SetFocalPoint( 8.88908, 0.595038, 29.3342 )
cam.SetPosition( -12.3332, 31.7479, 41.2387 )
cam.SetViewUp( 0.060772, -0.319905, 0.945498 )

renWin.Render()
