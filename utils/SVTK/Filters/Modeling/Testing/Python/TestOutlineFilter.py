#!/usr/bin/env python
import svtk

# Test svtkOutlineFilter with composite dataset input; test svtkCompositeOutlineFilter.

# Control test size
res = 50

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

# Create a composite dataset. Throw in an extra polydata.
sphere = svtk.svtkSphereSource()
sphere.SetCenter(1,0,0)
sphere.Update()

composite = svtk.svtkMultiBlockDataSet()
composite.SetBlock(0,extractL.GetOutput())
composite.SetBlock(1,extractR.GetOutput())
composite.SetBlock(2,sphere.GetOutput())

# Create an outline around everything (composite dataset)
outlineF = svtk.svtkOutlineFilter()
outlineF.SetInputData(composite)
outlineF.SetCompositeStyleToRoot()
outlineF.GenerateFacesOn()

outlineM = svtk.svtkPolyDataMapper()
outlineM.SetInputConnection(outlineF.GetOutputPort())

outline = svtk.svtkActor()
outline.SetMapper(outlineM)
outline.GetProperty().SetColor(1,0,0)

# Create an outline around composite pieces
coutlineF = svtk.svtkOutlineFilter()
coutlineF.SetInputData(composite)
coutlineF.SetCompositeStyleToLeafs()

coutlineM = svtk.svtkPolyDataMapper()
coutlineM.SetInputConnection(coutlineF.GetOutputPort())

coutline = svtk.svtkActor()
coutline.SetMapper(coutlineM)
coutline.GetProperty().SetColor(0,0,0)

# Create an outline around root and leafs
outlineF2 = svtk.svtkOutlineFilter()
outlineF2.SetInputData(composite)
outlineF2.SetCompositeStyleToRootAndLeafs()

outlineM2 = svtk.svtkPolyDataMapper()
outlineM2.SetInputConnection(outlineF2.GetOutputPort())

outline2 = svtk.svtkActor()
outline2.SetMapper(outlineM2)
outline2.GetProperty().SetColor(0,0,1)

# Create an outline around root and leafs
outlineF3 = svtk.svtkOutlineFilter()
outlineF3.SetInputData(composite)
outlineF3.SetCompositeStyleToSpecifiedIndex()
outlineF3.AddIndex(3)
outlineF3.GenerateFacesOn()

outlineM3 = svtk.svtkPolyDataMapper()
outlineM3.SetInputConnection(outlineF3.GetOutputPort())

outline3 = svtk.svtkActor()
outline3.SetMapper(outlineM3)
outline3.GetProperty().SetColor(0,1,0)

# Define graphics objects
renWin = svtk.svtkRenderWindow()
renWin.SetSize(600,150)
renWin.SetMultiSamples(0)

ren1 = svtk.svtkRenderer()
ren1.SetBackground(1,1,1)
ren1.SetViewport(0,0,0.25,1)
cam1 = ren1.GetActiveCamera()
ren2 = svtk.svtkRenderer()
ren2.SetBackground(1,1,1)
ren2.SetViewport(0.25,0,0.5,1)
ren2.SetActiveCamera(cam1)
ren3 = svtk.svtkRenderer()
ren3.SetBackground(1,1,1)
ren3.SetViewport(0.5,0,0.75,1)
ren3.SetActiveCamera(cam1)
ren4 = svtk.svtkRenderer()
ren4.SetBackground(1,1,1)
ren4.SetViewport(0.75,0,1,1)
ren4.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(outline)
ren2.AddActor(coutline)
ren3.AddActor(outline2)
ren4.AddActor(outline3)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
