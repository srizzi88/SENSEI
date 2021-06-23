#!/usr/bin/env python
############################################################
from __future__ import print_function
from svtk import *
from svtk.util.misc import svtkGetDataRoot
############################################################

# Create renderer and add actors to it
renderer = svtkRenderer()
renderer.SetBackground(.8, .8, .8)

# Create render window
window = svtkRenderWindow()
window.AddRenderer(renderer)
window.SetSize(500, 500)

# Create interactor
interactor = svtkRenderWindowInteractor()
interactor.SetRenderWindow(window)

# Retrieve data root
SVTK_DATA_ROOT = svtkGetDataRoot()
print(SVTK_DATA_ROOT)
# Read from AVS UCD data in binary form
reader = svtkAVSucdReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/UCD2D/UCD_00005.inp")

# Update reader and get mesh cell data
reader.Update()
mesh = reader.GetOutput()
cellData = mesh.GetCellData()

# Create normal vectors
cellData.SetActiveScalars("norme[0]")
normX = cellData.GetScalars()
cellData.SetActiveScalars("norme[1]")
normY = cellData.GetScalars()
n = normX.GetNumberOfTuples()
norm = svtkDoubleArray()
norm.SetNumberOfComponents(3)
norm.SetNumberOfTuples(n)
norm.SetName("norme")
for i in range(n):
    norm.SetTuple3(i, normX.GetTuple1( i ), normY.GetTuple1( i ), 0.0)
cellData.SetVectors(norm)

# Extract submeshes corresponding to 2 different material Ids
cellData.SetActiveScalars("Material Id")
threshold2 = svtkThreshold()
threshold2.SetInputData(mesh)
threshold2.SetInputArrayToProcess(0, 0, 0, svtkDataObject.FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes.SCALARS);
threshold2.ThresholdByLower(2)
threshold2.Update()
meshMat2 = threshold2.GetOutput()
threshold3 = svtkThreshold()
threshold3.SetInputData(mesh)
threshold3.SetInputArrayToProcess(0, 0, 0, svtkDataObject.FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes.SCALARS);
threshold3.ThresholdByUpper(3)
threshold3.Update()
meshMat3 = threshold3.GetOutput()

# Make multiblock from extracted submeshes
meshMB = svtkMultiBlockDataSet()
meshMB.SetNumberOfBlocks(1)
meshMB.GetMetaData(0).Set(svtkCompositeDataSet.NAME(), "Material 2")
meshMB.SetBlock(0, meshMat2)

# Create mapper for submesh corresponding to material 3
matRange = cellData.GetScalars().GetRange()
meshMapper = svtkDataSetMapper()
meshMapper.SetInputData(meshMat3)
meshMapper.SetScalarRange(matRange)
meshMapper.SetScalarModeToUseCellData()
meshMapper.SetColorModeToMapScalars()
meshMapper.ScalarVisibilityOn()
meshMapper.SetResolveCoincidentTopologyPolygonOffsetParameters(0, 1)
meshMapper.SetResolveCoincidentTopologyToPolygonOffset()

# Create wireframe actor for submesh
meshActor = svtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
renderer.AddViewProp(meshActor)

cellData.SetActiveScalars("frac_pres[1]")
# Reconstruct material interface
interface = svtkYoungsMaterialInterface()
interface.SetInputData(meshMB)
interface.SetNumberOfMaterials(1)
interface.SetMaterialVolumeFractionArray(0, "frac_pres[1]")
interface.SetMaterialNormalArray(0, "norme")
interface.FillMaterialOn()
interface.UseAllBlocksOn()
interface.Update()

# Create mappers and actors for surface rendering of all reconstructed interfaces
interfaceIterator = svtkDataObjectTreeIterator()
interfaceIterator.SetDataSet(interface.GetOutput())
interfaceIterator.VisitOnlyLeavesOn()
interfaceIterator.SkipEmptyNodesOn()
interfaceIterator.InitTraversal()
interfaceIterator.GoToFirstItem()
while (interfaceIterator.IsDoneWithTraversal() == 0):
    idx = interfaceIterator.GetCurrentFlatIndex()
    # Create mapper for leaf node
    print("Creating mapper and actor for object with flat index", idx)
    interfaceMapper = svtkDataSetMapper()
    interfaceMapper.SetInputData(interfaceIterator.GetCurrentDataObject())
    interfaceIterator.GoToNextItem()
    interfaceMapper.ScalarVisibilityOff()
    interfaceMapper.SetResolveCoincidentTopologyPolygonOffsetParameters(1, 1)
    interfaceMapper.SetResolveCoincidentTopologyToPolygonOffset()
    # Create surface actor and add it to view
    interfaceActor = svtkActor()
    interfaceActor.SetMapper(interfaceMapper)
    if (idx == 2):
        interfaceActor.GetProperty().SetColor(0, 1, 0)
    else:
        interfaceActor.GetProperty().SetColor(0, 0, 1)
    renderer.AddViewProp(interfaceActor)

# Start interaction
window.Render()
interactor.Start()
