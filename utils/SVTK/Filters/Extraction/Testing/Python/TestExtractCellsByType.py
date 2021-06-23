#!/usr/bin/env python
import svtk

# Test cell extraction by type by creating a hodgepodge of
# different datasets with different types. Then extracting
# cells and making sure the count is correct.

# Control test size
res = 20
error = 0

# Create an initial volume and extract different types of cells out of it.
#
# Quadric definition
quadric = svtk.svtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])

sample = svtk.svtkSampleFunction()
sample.SetSampleDimensions((res+1),(res+1),(res+1))
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOn()
sample.Update()

# Extract cells - should be res*res*res voxels
extr = svtk.svtkExtractCellsByType()
extr.SetInputConnection(sample.GetOutputPort())
extr.AddCellType(svtk.SVTK_VOXEL)
extr.Update()
print("Number of voxels: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

# Extract hex cells - should be 0 hexes
extr.RemoveAllCellTypes()
extr.AddCellType(svtk.SVTK_HEXAHEDRON)
extr.Update()
print("Number of hexes: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 0:
    error = 1

# Generate tetrahedral mesh. The side effect of the clip filter
# is to produce tetrahedra.
clip = svtk.svtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetValue(-10.0)
clip.GenerateClippedOutputOff()
clip.Update()

# Extract tetra cells as unstructured grid - should be 5*res*res*res tets
extr.SetInputConnection(clip.GetOutputPort())
extr.AddCellType(svtk.SVTK_TETRA)
extr.Update()
print("Number of tets: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 5*res*res*res:
    error = 1

# Extract wedge cells as unstructured grid - should be 0 wedges
extr.RemoveCellType(svtk.SVTK_TETRA)
extr.AddCellType(svtk.SVTK_WEDGE)
extr.Update()
print("Number of wedges: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 0:
    error = 1

# Now construct unstructured grid as conglomerate of tets and voxels. Should
# produce res*res*res + 5*res*res*res cells
appendU = svtk.svtkAppendFilter()
appendU.AddInputConnection(sample.GetOutputPort())
appendU.AddInputConnection(clip.GetOutputPort())

extr.SetInputConnection(appendU.GetOutputPort())
extr.AddAllCellTypes()
extr.Update()
print("Number of unstructured cells: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 6*res*res*res:
    error = 1

extr.RemoveAllCellTypes()
extr.AddCellType(svtk.SVTK_TETRA)
extr.Update()
print("\tNumber of tets: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 5*res*res*res:
    error = 1

extr.RemoveAllCellTypes()
extr.AddCellType(svtk.SVTK_VOXEL)
extr.Update()
print("\tNumber of voxels: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

# Now construct polydata as a conglomerate of verts, lines, polys
centers = svtk.svtkCellCenters()
centers.SetInputConnection(sample.GetOutputPort())
centers.VertexCellsOn()

edges = svtk.svtkExtractEdges()
edges.SetInputConnection(sample.GetOutputPort())

polys = svtk.svtkGeometryFilter()
polys.SetInputConnection(sample.GetOutputPort())

appendP = svtk.svtkAppendPolyData()
appendP.AddInputConnection(centers.GetOutputPort())
appendP.AddInputConnection(edges.GetOutputPort())
appendP.AddInputConnection(polys.GetOutputPort())
appendP.Update()

extr.SetInputConnection(appendP.GetOutputPort())
extr.AddAllCellTypes()
extr.Update()
print("Number of polydata cells: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != (res*res*res) + 3*res*(res+1)*(res+1) + 6*res*res:
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveAllCellTypes()
extr.AddCellType(svtk.SVTK_VERTEX)
extr.Update()
print("\tNumber of verts: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != res*res*res:
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveCellType(svtk.SVTK_VERTEX)
extr.AddCellType(svtk.SVTK_LINE)
extr.Update()
print("\tNumber of lines: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 3*res*(res+1)*(res+1):
    error = 1

extr.SetInputConnection(appendP.GetOutputPort())
extr.RemoveCellType(svtk.SVTK_LINE)
extr.AddCellType(svtk.SVTK_QUAD)
extr.Update()
print("\tNumber of polys: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 6*res*res:
    error = 1

# Finally triangle strips. The number of triangle strips is empiraclly determined.
tris = svtk.svtkTriangleFilter()
tris.SetInputConnection(polys.GetOutputPort())

stripper = svtk.svtkStripper()
stripper.SetInputConnection(tris.GetOutputPort())

extr.SetInputConnection(stripper.GetOutputPort())
extr.RemoveCellType(svtk.SVTK_LINE)
extr.AddCellType(svtk.SVTK_TRIANGLE_STRIP)
extr.Update()
print("Number of triangle strips: {0}".format(extr.GetOutput().GetNumberOfCells()))
if extr.GetOutput().GetNumberOfCells() != 153:
    error = 1


# Return test results. If the assert is not true, then different results were
# generated by svtkStaticPointLocator and svtkPointLocator.
assert error == 0

# --- end of script --
