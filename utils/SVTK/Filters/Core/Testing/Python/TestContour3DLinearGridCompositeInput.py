#!/usr/bin/env python
import svtk

# Test svtkContour3DLinearGrid with svtkScalarTree and an input
# svtkCompositeDataSet

# Control test size
res = 50
#res = 250
serialProcessing = 0
mergePoints = 1
interpolateAttr = 0
computeNormals = 0
computeScalarRange = 0

#
# Quadric definition
quadricL = svtk.svtkQuadric()
quadricL.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleL = svtk.svtkSampleFunction()
sampleL.SetModelBounds(-1,0, -1,1, -1,1)
sampleL.SetSampleDimensions(int(res/2),res,res)
sampleL.SetImplicitFunction(quadricL)
sampleL.ComputeNormalsOn()
sampleL.Update()

#
# Quadric definition
quadricR = svtk.svtkQuadric()
quadricR.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleR = svtk.svtkSampleFunction()
sampleR.SetModelBounds(0,1, -1,1, -1,1)
sampleR.SetSampleDimensions(int(res/2),res,res)
sampleR.SetImplicitFunction(quadricR)
sampleR.ComputeNormalsOn()
sampleR.Update()

#
# Extract voxel cells
extractL = svtk.svtkExtractCells()
extractL.SetInputConnection(sampleL.GetOutputPort())
extractL.AddCellRange(0,sampleL.GetOutput().GetNumberOfCells())
extractL.Update()

#
# Extract voxel cells
extractR = svtk.svtkExtractCells()
extractR.SetInputConnection(sampleR.GetOutputPort())
extractR.AddCellRange(0,sampleR.GetOutput().GetNumberOfCells())
extractR.Update()

# Create a composite dataset. Throw in an extra polydata which should be skipped.
sphere = svtk.svtkSphereSource()
sphere.SetCenter(1,0,0)
sphere.Update()

composite = svtk.svtkMultiBlockDataSet()
composite.SetBlock(0,extractL.GetOutput())
composite.SetBlock(1,extractR.GetOutput())
composite.SetBlock(2,sphere.GetOutput())

# Scalar tree is used to clone for each of the composite pieces
stree = svtk.svtkSpanSpace()
stree.SetDataSet(extractL.GetOutput())
stree.ComputeResolutionOn()
stree.SetScalarRange(0.25,0.75)
stree.SetComputeScalarRange(computeScalarRange)

# Since there is a svtkPolyData added to the composite input, this dataset
# cannot be fully processed by svtkContour3DLinearGrid
assert(not svtk.svtkContour3DLinearGrid.CanFullyProcessDataObject(composite, "scalars"))

# Now contour the cells, using scalar tree
contour = svtk.svtkContour3DLinearGrid()
contour.SetInputData(composite)
contour.SetValue(0, 0.5)
contour.SetMergePoints(mergePoints)
contour.SetSequentialProcessing(serialProcessing)
contour.SetInterpolateAttributes(interpolateAttr);
contour.SetComputeNormals(computeNormals);
contour.UseScalarTreeOn()
contour.SetScalarTree(stree)

contMapper = svtk.svtkCompositePolyDataMapper()
contMapper.SetInputConnection(contour.GetOutputPort())
contMapper.ScalarVisibilityOff()

contActor = svtk.svtkActor()
contActor.SetMapper(contMapper)
contActor.GetProperty().SetColor(.8,.4,.4)

# Now contour the cells, using scalar tree
outlineF = svtk.svtkOutlineFilter()
outlineF.SetInputData(composite)

outlineM = svtk.svtkPolyDataMapper()
outlineM.SetInputConnection(outlineF.GetOutputPort())

outline = svtk.svtkActor()
outline.SetMapper(outlineM)
outline.GetProperty().SetColor(0,0,0)

# Timing comparisons
timer = svtk.svtkTimerLog()

timer.StartTimer()
contour.Update()
timer.StopTimer()
streetime = timer.GetElapsedTime()

timer.StartTimer()
contour.Modified()
contour.Update()
timer.StopTimer()
time = timer.GetElapsedTime()

print("Contouring comparison")
print("\tScalar Range: {}".format(stree.GetScalarRange()))
print("\tNumber of threads used: {0}".format(contour.GetNumberOfThreadsUsed()))
print("\tMerge points: {0}".format(mergePoints))
print("\tInterpolate attributes: {0}".format(interpolateAttr))
print("\tCompute Normals: {0}".format(computeNormals))

print("\tLinear 3D contouring (Build scalar tree): {0}".format(streetime))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

print("\tLinear 3D contouring (With built scalar tree): {0}".format(time))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

# Define graphics objects
renWin = svtk.svtkRenderWindow()
renWin.SetSize(200,200)
renWin.SetMultiSamples(0)

ren1 = svtk.svtkRenderer()
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()

renWin.AddRenderer(ren1)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(contActor)
ren1.AddActor(outline)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
