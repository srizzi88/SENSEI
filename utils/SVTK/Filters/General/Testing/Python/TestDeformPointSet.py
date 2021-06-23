#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
renderer = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(renderer)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a sphere to warp
sphere = svtk.svtkSphereSource()
sphere.SetThetaResolution(51)
sphere.SetPhiResolution(17)

# Generate some scalars on the sphere
ele = svtk.svtkElevationFilter()
ele.SetInputConnection(sphere.GetOutputPort())
ele.SetLowPoint(0, 0, -0.5)
ele.SetHighPoint(0, 0, 0.5)

# Now create a control mesh, in this case a octagon
pts = svtk.svtkPoints()
pts.SetNumberOfPoints(6)
pts.SetPoint(0, -1, 0, 0)
pts.SetPoint(1, 1, 0, 0)
pts.SetPoint(2, 0, -1, 0)
pts.SetPoint(3, 0, 1, 0)
pts.SetPoint(4, 0, 0, -1)
pts.SetPoint(5, 0, 0, 1)

tris = svtk.svtkCellArray()
tris.InsertNextCell(3)
tris.InsertCellPoint(2)
tris.InsertCellPoint(0)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(2)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(3)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(2)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(2)
tris.InsertCellPoint(1)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(3)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(5)

pd = svtk.svtkPolyData()
pd.SetPoints(pts)
pd.SetPolys(tris)

# Display the control mesh
meshMapper = svtk.svtkPolyDataMapper()
meshMapper.SetInputData(pd)

meshActor = svtk.svtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
meshActor.GetProperty().SetColor(0, 0, 0)

# Okay now let's do the initial weight generation
deform = svtk.svtkDeformPointSet()
deform.SetInputConnection(ele.GetOutputPort())
deform.SetControlMeshData(pd)
deform.Update()  # this creates the initial weights

# Now move one point and deform
pts.SetPoint(5, 0, 0, 3)
pts.Modified()
deform.Update()

# Display the warped sphere
sphereMapper = svtk.svtkPolyDataMapper()
sphereMapper.SetInputConnection(deform.GetOutputPort())

sphereActor = svtk.svtkActor()
sphereActor.SetMapper(sphereMapper)

renderer.AddActor(sphereActor)
renderer.AddActor(meshActor)
renderer.GetActiveCamera().SetPosition(1, 1, 1)
renderer.ResetCamera()

renderer.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)

# interact with data
renWin.Render()
iren.Initialize()
# --- end of script --
