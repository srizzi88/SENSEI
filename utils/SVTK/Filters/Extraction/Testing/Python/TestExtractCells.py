#!/usr/bin/env python
import svtk

# Test svtkExtractCells
# Control test size
#res = 200
res = 50

# Test cell extraction
#
# Quadric definition
quadric = svtk.svtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = svtk.svtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()

# Now extract the cells: use badly formed range
extract = svtk.svtkExtractCells()
extract.SetInputConnection(sample.GetOutputPort())
extract.AddCellRange(0,sample.GetOutput().GetNumberOfCells())

extrMapper = svtk.svtkDataSetMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOff()

extrActor = svtk.svtkActor()
extrActor.SetMapper(extrMapper)
extrActor.GetProperty().SetColor(.8,.4,.4)

# Extract interior range of cells
extract2 = svtk.svtkExtractCells()
extract2.SetInputConnection(sample.GetOutputPort())
extract2.AddCellRange(100,5000)
extract2.Update()

extr2Mapper = svtk.svtkDataSetMapper()
extr2Mapper.SetInputConnection(extract2.GetOutputPort())
extr2Mapper.ScalarVisibilityOff()

extr2Actor = svtk.svtkActor()
extr2Actor.SetMapper(extr2Mapper)
extr2Actor.GetProperty().SetColor(.8,.4,.4)

# Define graphics objects
renWin = svtk.svtkRenderWindow()
renWin.SetSize(300,150)

ren1 = svtk.svtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()
ren2 = svtk.svtkRenderer()
ren2.SetViewport(0.5,0,1,1)
ren2.SetBackground(1,1,1)
ren2.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(extrActor)
ren2.AddActor(extr2Actor)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
