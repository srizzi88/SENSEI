#!/usr/bin/env python
import svtk

# Parameters for controlling the test size
dim = 40
numStreamlines = 50

# Create three volumes of different resolution and butt them together.  This
# tests svtkPointSet::FindCell() on incompatiable meshes (hanging and
# duplicate nodes).

spacing = 1.0 / (2.0*dim - 1.0)
v1 = svtk.svtkImageData()
v1.SetOrigin(0.0,0,0)
v1.SetDimensions(2*dim,2*dim,2*dim)
v1.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (dim - 1.0)
v2 = svtk.svtkImageData()
v2.SetOrigin(1.0,0,0)
v2.SetDimensions(dim,dim,dim)
v2.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (2.0*dim - 1.0)
v3 = svtk.svtkImageData()
v3.SetOrigin(2.0,0,0)
v3.SetDimensions(2*dim,2*dim,2*dim)
v3.SetSpacing(spacing,spacing,spacing)

# Append the volumes together to create an unstructured grid with duplicate
# and hanging points.
append = svtk.svtkAppendFilter()
append.AddInputData(v1)
append.AddInputData(v2)
append.AddInputData(v3)
append.MergePointsOff()
append.Update()

# Create a uniform vector field in the x-direction
numPts = append.GetOutput().GetNumberOfPoints()
vectors = svtk.svtkFloatArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(numPts);
for i in range(0,numPts):
    vectors.SetTuple3(i, 1,0,0)

# A hack but it works
append.GetOutput().GetPointData().SetVectors(vectors)

# Outline around appended unstructured grid
outline = svtk.svtkOutlineFilter()
outline.SetInputConnection(append.GetOutputPort())

outlineMapper = svtk.svtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = svtk.svtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create streamlines from a rake of seed points
pt1 = [0.001,0.1,0.5]
pt2 = [0.001,0.9,0.5]
line = svtk.svtkLineSource()
line.SetResolution(numStreamlines-1)
line.SetPoint1(pt1)
line.SetPoint2(pt2)
line.Update()

rk4 = svtk.svtkRungeKutta4()
strategy = svtk.svtkClosestNPointsStrategy()
ivp = svtk.svtkInterpolatedVelocityField()
ivp.SetFindCellStrategy(strategy)

streamer = svtk.svtkStreamTracer()
streamer.SetInputConnection(append.GetOutputPort())
streamer.SetSourceConnection(line.GetOutputPort())
streamer.SetMaximumPropagation(10)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetMinimumIntegrationStep(0.01)
streamer.SetMaximumIntegrationStep(0.5)
streamer.SetTerminalSpeed(1.0e-12)
streamer.SetMaximumError(1.0e-06)
streamer.SetComputeVorticity(0)
streamer.SetIntegrator(rk4)
streamer.SetInterpolatorPrototype(ivp)
streamer.Update()
reasons = streamer.GetOutput().GetCellData().GetArray("ReasonForTermination")
print(reasons.GetValue(0))

strMapper = svtk.svtkPolyDataMapper()
strMapper.SetInputConnection(streamer.GetOutputPort())

strActor = svtk.svtkActor()
strActor.SetMapper(strMapper)

# rendering
ren = svtk.svtkRenderer()
renWin = svtk.svtkRenderWindow()
iren = svtk.svtkRenderWindowInteractor()
renWin.AddRenderer(ren)
iren.SetRenderWindow(renWin)

ren.AddActor(outlineActor)
ren.AddActor(strActor)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2)
renWin.SetSize(600, 300)
renWin.Render()

iren.Start()
