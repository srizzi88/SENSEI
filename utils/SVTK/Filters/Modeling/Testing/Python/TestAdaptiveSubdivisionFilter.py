#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create a pipeline: some skinny-ass triangles
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(6)
sphere.SetPhiResolution(24)

ids = svtk.svtkIdFilter()
ids.SetInputConnection(sphere.GetOutputPort())

adapt = svtk.svtkAdaptiveSubdivisionFilter()
adapt.SetInputConnection(ids.GetOutputPort())
adapt.SetMaximumEdgeLength(0.1)
#adapt.SetMaximumTriangleArea(0.01)
#adapt.SetMaximumNumberOfPasses(1)
adapt.Update()

mapper = svtk.svtkPolyDataMapper()
mapper.SetInputConnection(adapt.GetOutputPort())
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray("svtkIdFilter_Ids")
mapper.SetScalarRange(adapt.GetOutput().GetCellData().GetScalars().GetRange())

edgeProp = svtk.svtkProperty()
edgeProp.EdgeVisibilityOn()
edgeProp.SetEdgeColor(0,0,0)

actor = svtk.svtkActor()
actor.SetMapper(mapper)
actor.SetProperty(edgeProp)

# Graphics stuff
ren1 = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)

renWin.SetSize(300,300)
renWin.Render()
#iren.Start()
