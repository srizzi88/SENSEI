#!/usr/bin/env python
import sys
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Prevent .pyc files being created.
# Stops the svtk source being polluted
# by .pyc files.
sys.dont_write_bytecode = True

import backdrop

# Contour every cell type

# Since some of our actors are a single vertex, we need to remove all
# cullers so the single vertex actors will render
ren1 = svtk.svtkRenderer()
ren1.GetCullers().RemoveAllItems()

renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a scene with one of each cell type
# Voxel
voxelPoints = svtk.svtkPoints()
voxelPoints.SetNumberOfPoints(8)
voxelPoints.InsertPoint(0, 0, 0, 0)
voxelPoints.InsertPoint(1, 1, 0, 0)
voxelPoints.InsertPoint(2, 0, 1, 0)
voxelPoints.InsertPoint(3, 1, 1, 0)
voxelPoints.InsertPoint(4, 0, 0, 1)
voxelPoints.InsertPoint(5, 1, 0, 1)
voxelPoints.InsertPoint(6, 0, 1, 1)
voxelPoints.InsertPoint(7, 1, 1, 1)

voxelScalars = svtk.svtkFloatArray()
voxelScalars.SetNumberOfTuples(8)
voxelScalars.InsertValue(0, 0)
voxelScalars.InsertValue(1, 1)
voxelScalars.InsertValue(2, 0)
voxelScalars.InsertValue(3, 0)
voxelScalars.InsertValue(4, 0)
voxelScalars.InsertValue(5, 0)
voxelScalars.InsertValue(6, 0)
voxelScalars.InsertValue(7, 0)

aVoxel = svtk.svtkVoxel()
aVoxel.GetPointIds().SetId(0, 0)
aVoxel.GetPointIds().SetId(1, 1)
aVoxel.GetPointIds().SetId(2, 2)
aVoxel.GetPointIds().SetId(3, 3)
aVoxel.GetPointIds().SetId(4, 4)
aVoxel.GetPointIds().SetId(5, 5)
aVoxel.GetPointIds().SetId(6, 6)
aVoxel.GetPointIds().SetId(7, 7)

aVoxelGrid = svtk.svtkUnstructuredGrid()
aVoxelGrid.Allocate(1, 1)
aVoxelGrid.InsertNextCell(aVoxel.GetCellType(), aVoxel.GetPointIds())
aVoxelGrid.SetPoints(voxelPoints)
aVoxelGrid.GetPointData().SetScalars(voxelScalars)

voxelContours = svtk.svtkContourFilter()
voxelContours.SetInputData(aVoxelGrid)
voxelContours.SetValue(0, .5)

aVoxelContourMapper = svtk.svtkDataSetMapper()
aVoxelContourMapper.SetInputConnection(voxelContours.GetOutputPort())
aVoxelContourMapper.ScalarVisibilityOff()

aVoxelMapper = svtk.svtkDataSetMapper()
aVoxelMapper.SetInputData(aVoxelGrid)
aVoxelMapper.ScalarVisibilityOff()

aVoxelActor = svtk.svtkActor()
aVoxelActor.SetMapper(aVoxelMapper)
aVoxelActor.GetProperty().SetRepresentationToWireframe()

aVoxelContourActor = svtk.svtkActor()
aVoxelContourActor.SetMapper(aVoxelContourMapper)
aVoxelContourActor.GetProperty().BackfaceCullingOn()

# Hexahedron

hexahedronPoints = svtk.svtkPoints()
hexahedronPoints.SetNumberOfPoints(8)
hexahedronPoints.InsertPoint(0, 0, 0, 0)
hexahedronPoints.InsertPoint(1, 1, 0, 0)
hexahedronPoints.InsertPoint(2, 1, 1, 0)
hexahedronPoints.InsertPoint(3, 0, 1, 0)
hexahedronPoints.InsertPoint(4, 0, 0, 1)
hexahedronPoints.InsertPoint(5, 1, 0, 1)
hexahedronPoints.InsertPoint(6, 1, 1, 1)
hexahedronPoints.InsertPoint(7, 0, 1, 1)

hexahedronScalars = svtk.svtkFloatArray()
hexahedronScalars.SetNumberOfTuples(8)
hexahedronScalars.InsertValue(0, 0)
hexahedronScalars.InsertValue(1, 1)
hexahedronScalars.InsertValue(2, 0)
hexahedronScalars.InsertValue(3, 0)
hexahedronScalars.InsertValue(4, 0)
hexahedronScalars.InsertValue(5, 0)
hexahedronScalars.InsertValue(6, 0)
hexahedronScalars.InsertValue(7, 0)

aHexahedron = svtk.svtkHexahedron()
aHexahedron.GetPointIds().SetId(0, 0)
aHexahedron.GetPointIds().SetId(1, 1)
aHexahedron.GetPointIds().SetId(2, 2)
aHexahedron.GetPointIds().SetId(3, 3)
aHexahedron.GetPointIds().SetId(4, 4)
aHexahedron.GetPointIds().SetId(5, 5)
aHexahedron.GetPointIds().SetId(6, 6)
aHexahedron.GetPointIds().SetId(7, 7)

aHexahedronGrid = svtk.svtkUnstructuredGrid()
aHexahedronGrid.Allocate(1, 1)
aHexahedronGrid.InsertNextCell(aHexahedron.GetCellType(), aHexahedron.GetPointIds())
aHexahedronGrid.SetPoints(hexahedronPoints)
aHexahedronGrid.GetPointData().SetScalars(hexahedronScalars)

hexahedronContours = svtk.svtkContourFilter()
hexahedronContours.SetInputData(aHexahedronGrid)
hexahedronContours.SetValue(0, .5)

aHexahedronContourMapper = svtk.svtkDataSetMapper()
aHexahedronContourMapper.SetInputConnection(hexahedronContours.GetOutputPort())
aHexahedronContourMapper.ScalarVisibilityOff()

aHexahedronMapper = svtk.svtkDataSetMapper()
aHexahedronMapper.SetInputData(aHexahedronGrid)
aHexahedronMapper.ScalarVisibilityOff()

aHexahedronActor = svtk.svtkActor()
aHexahedronActor.SetMapper(aHexahedronMapper)
aHexahedronActor.GetProperty().BackfaceCullingOn()
aHexahedronActor.GetProperty().SetRepresentationToWireframe()

aHexahedronContourActor = svtk.svtkActor()
aHexahedronContourActor.SetMapper(aHexahedronContourMapper)
aHexahedronContourActor.GetProperty().BackfaceCullingOn()

# Tetra

tetraPoints = svtk.svtkPoints()
tetraPoints.SetNumberOfPoints(4)
tetraPoints.InsertPoint(0, 0, 0, 0)
tetraPoints.InsertPoint(1, 1, 0, 0)
tetraPoints.InsertPoint(2, .5, 1, 0)
tetraPoints.InsertPoint(3, .5, .5, 1)

tetraScalars = svtk.svtkFloatArray()
tetraScalars.SetNumberOfTuples(4)
tetraScalars.InsertValue(0, 1)
tetraScalars.InsertValue(1, 0)
tetraScalars.InsertValue(2, 0)
tetraScalars.InsertValue(3, 0)

aTetra = svtk.svtkTetra()
aTetra.GetPointIds().SetId(0, 0)
aTetra.GetPointIds().SetId(1, 1)
aTetra.GetPointIds().SetId(2, 2)
aTetra.GetPointIds().SetId(3, 3)

aTetraGrid = svtk.svtkUnstructuredGrid()
aTetraGrid.Allocate(1, 1)
aTetraGrid.InsertNextCell(aTetra.GetCellType(), aTetra.GetPointIds())
aTetraGrid.SetPoints(tetraPoints)
aTetraGrid.GetPointData().SetScalars(tetraScalars)

tetraContours = svtk.svtkContourFilter()
tetraContours.SetInputData(aTetraGrid)
tetraContours.SetValue(0, .5)

aTetraContourMapper = svtk.svtkDataSetMapper()
aTetraContourMapper.SetInputConnection(tetraContours.GetOutputPort())
aTetraContourMapper.ScalarVisibilityOff()

aTetraMapper = svtk.svtkDataSetMapper()
aTetraMapper.SetInputData(aTetraGrid)
aTetraMapper.ScalarVisibilityOff()

aTetraContourActor = svtk.svtkActor()
aTetraContourActor.SetMapper(aTetraContourMapper)

aTetraActor = svtk.svtkActor()
aTetraActor.SetMapper(aTetraMapper)
aTetraActor.GetProperty().SetRepresentationToWireframe()

# Wedge

wedgePoints = svtk.svtkPoints()
wedgePoints.SetNumberOfPoints(6)
wedgePoints.InsertPoint(0, 0, 1, 0)
wedgePoints.InsertPoint(1, 0, 0, 0)
wedgePoints.InsertPoint(2, 0, .5, .5)
wedgePoints.InsertPoint(3, 1, 1, 0)
wedgePoints.InsertPoint(4, 1, 0, 0)
wedgePoints.InsertPoint(5, 1, .5, .5)

wedgeScalars = svtk.svtkFloatArray()
wedgeScalars.SetNumberOfTuples(6)
wedgeScalars.InsertValue(0, 1)
wedgeScalars.InsertValue(1, 1)
wedgeScalars.InsertValue(2, 0)
wedgeScalars.InsertValue(3, 1)
wedgeScalars.InsertValue(4, 1)
wedgeScalars.InsertValue(5, 0)

aWedge = svtk.svtkWedge()
aWedge.GetPointIds().SetId(0, 0)
aWedge.GetPointIds().SetId(1, 1)
aWedge.GetPointIds().SetId(2, 2)
aWedge.GetPointIds().SetId(3, 3)
aWedge.GetPointIds().SetId(4, 4)
aWedge.GetPointIds().SetId(5, 5)

aWedgeGrid = svtk.svtkUnstructuredGrid()
aWedgeGrid.Allocate(1, 1)
aWedgeGrid.InsertNextCell(aWedge.GetCellType(), aWedge.GetPointIds())
aWedgeGrid.SetPoints(wedgePoints)
aWedgeGrid.GetPointData().SetScalars(wedgeScalars)

wedgeContours = svtk.svtkContourFilter()
wedgeContours.SetInputData(aWedgeGrid)
wedgeContours.SetValue(0, .5)

aWedgeContourMapper = svtk.svtkDataSetMapper()
aWedgeContourMapper.SetInputConnection(wedgeContours.GetOutputPort())
aWedgeContourMapper.ScalarVisibilityOff()

aWedgeMapper = svtk.svtkDataSetMapper()
aWedgeMapper.SetInputData(aWedgeGrid)
aWedgeMapper.ScalarVisibilityOff()

aWedgeContourActor = svtk.svtkActor()
aWedgeContourActor.SetMapper(aWedgeContourMapper)

aWedgeActor = svtk.svtkActor()
aWedgeActor.SetMapper(aWedgeMapper)
aWedgeActor.GetProperty().SetRepresentationToWireframe()

# Pyramid

pyramidPoints = svtk.svtkPoints()
pyramidPoints.SetNumberOfPoints(5)
pyramidPoints.InsertPoint(0, 0, 0, 0)
pyramidPoints.InsertPoint(1, 1, 0, 0)
pyramidPoints.InsertPoint(2, 1, 1, 0)
pyramidPoints.InsertPoint(3, 0, 1, 0)
pyramidPoints.InsertPoint(4, .5, .5, 1)

pyramidScalars = svtk.svtkFloatArray()
pyramidScalars.SetNumberOfTuples(5)
pyramidScalars.InsertValue(0, 1)
pyramidScalars.InsertValue(1, 1)
pyramidScalars.InsertValue(2, 1)
pyramidScalars.InsertValue(3, 1)
pyramidScalars.InsertValue(4, 0)

aPyramid = svtk.svtkPyramid()
aPyramid.GetPointIds().SetId(0, 0)
aPyramid.GetPointIds().SetId(1, 1)
aPyramid.GetPointIds().SetId(2, 2)
aPyramid.GetPointIds().SetId(3, 3)
aPyramid.GetPointIds().SetId(4, 4)

aPyramidGrid = svtk.svtkUnstructuredGrid()
aPyramidGrid.Allocate(1, 1)
aPyramidGrid.InsertNextCell(aPyramid.GetCellType(), aPyramid.GetPointIds())
aPyramidGrid.SetPoints(pyramidPoints)
aPyramidGrid.GetPointData().SetScalars(pyramidScalars)

pyramidContours = svtk.svtkContourFilter()
pyramidContours.SetInputData(aPyramidGrid)
pyramidContours.SetValue(0, .5)

aPyramidContourMapper = svtk.svtkDataSetMapper()
aPyramidContourMapper.SetInputConnection(pyramidContours.GetOutputPort())
aPyramidContourMapper.ScalarVisibilityOff()

aPyramidMapper = svtk.svtkDataSetMapper()
aPyramidMapper.SetInputData(aPyramidGrid)
aPyramidMapper.ScalarVisibilityOff()

aPyramidContourActor = svtk.svtkActor()
aPyramidContourActor.SetMapper(aPyramidContourMapper)

aPyramidActor = svtk.svtkActor()
aPyramidActor.SetMapper(aPyramidMapper)
aPyramidActor.GetProperty().SetRepresentationToWireframe()

# Pixel

pixelPoints = svtk.svtkPoints()
pixelPoints.SetNumberOfPoints(4)
pixelPoints.InsertPoint(0, 0, 0, 0)
pixelPoints.InsertPoint(1, 1, 0, 0)
pixelPoints.InsertPoint(2, 0, 1, 0)
pixelPoints.InsertPoint(3, 1, 1, 0)

pixelScalars = svtk.svtkFloatArray()
pixelScalars.SetNumberOfTuples(4)
pixelScalars.InsertValue(0, 1)
pixelScalars.InsertValue(1, 0)
pixelScalars.InsertValue(2, 0)
pixelScalars.InsertValue(3, 0)

aPixel = svtk.svtkPixel()
aPixel.GetPointIds().SetId(0, 0)
aPixel.GetPointIds().SetId(1, 1)
aPixel.GetPointIds().SetId(2, 2)
aPixel.GetPointIds().SetId(3, 3)

aPixelGrid = svtk.svtkUnstructuredGrid()
aPixelGrid.Allocate(1, 1)
aPixelGrid.InsertNextCell(aPixel.GetCellType(), aPixel.GetPointIds())
aPixelGrid.SetPoints(pixelPoints)
aPixelGrid.GetPointData().SetScalars(pixelScalars)

pixelContours = svtk.svtkContourFilter()
pixelContours.SetInputData(aPixelGrid)
pixelContours.SetValue(0, .5)

aPixelContourMapper = svtk.svtkDataSetMapper()
aPixelContourMapper.SetInputConnection(pixelContours.GetOutputPort())
aPixelContourMapper.ScalarVisibilityOff()

aPixelMapper = svtk.svtkDataSetMapper()
aPixelMapper.SetInputData(aPixelGrid)
aPixelMapper.ScalarVisibilityOff()

aPixelContourActor = svtk.svtkActor()
aPixelContourActor.SetMapper(aPixelContourMapper)

aPixelActor = svtk.svtkActor()
aPixelActor.SetMapper(aPixelMapper)
aPixelActor.GetProperty().BackfaceCullingOn()
aPixelActor.GetProperty().SetRepresentationToWireframe()

# Quad

quadPoints = svtk.svtkPoints()
quadPoints.SetNumberOfPoints(4)
quadPoints.InsertPoint(0, 0, 0, 0)
quadPoints.InsertPoint(1, 1, 0, 0)
quadPoints.InsertPoint(2, 1, 1, 0)
quadPoints.InsertPoint(3, 0, 1, 0)

quadScalars = svtk.svtkFloatArray()
quadScalars.SetNumberOfTuples(4)
quadScalars.InsertValue(0, 1)
quadScalars.InsertValue(1, 0)
quadScalars.InsertValue(2, 0)
quadScalars.InsertValue(3, 0)

aQuad = svtk.svtkQuad()
aQuad.GetPointIds().SetId(0, 0)
aQuad.GetPointIds().SetId(1, 1)
aQuad.GetPointIds().SetId(2, 2)
aQuad.GetPointIds().SetId(3, 3)

aQuadGrid = svtk.svtkUnstructuredGrid()
aQuadGrid.Allocate(1, 1)
aQuadGrid.InsertNextCell(aQuad.GetCellType(), aQuad.GetPointIds())
aQuadGrid.SetPoints(quadPoints)
aQuadGrid.GetPointData().SetScalars(quadScalars)

quadContours = svtk.svtkContourFilter()
quadContours.SetInputData(aQuadGrid)
quadContours.SetValue(0, .5)

aQuadContourMapper = svtk.svtkDataSetMapper()
aQuadContourMapper.SetInputConnection(quadContours.GetOutputPort())
aQuadContourMapper.ScalarVisibilityOff()

aQuadMapper = svtk.svtkDataSetMapper()
aQuadMapper.SetInputData(aQuadGrid)
aQuadMapper.ScalarVisibilityOff()

aQuadContourActor = svtk.svtkActor()
aQuadContourActor.SetMapper(aQuadContourMapper)

aQuadActor = svtk.svtkActor()
aQuadActor.SetMapper(aQuadMapper)
aQuadActor.GetProperty().BackfaceCullingOn()
aQuadActor.GetProperty().SetRepresentationToWireframe()

# Triangle

trianglePoints = svtk.svtkPoints()
trianglePoints.SetNumberOfPoints(3)
trianglePoints.InsertPoint(0, 0, 0, 0)
trianglePoints.InsertPoint(1, 1, 0, 0)
trianglePoints.InsertPoint(2, .5, .5, 0)

triangleScalars = svtk.svtkFloatArray()
triangleScalars.SetNumberOfTuples(3)
triangleScalars.InsertValue(0, 1)
triangleScalars.InsertValue(1, 0)
triangleScalars.InsertValue(2, 0)

aTriangle = svtk.svtkTriangle()
aTriangle.GetPointIds().SetId(0, 0)
aTriangle.GetPointIds().SetId(1, 1)
aTriangle.GetPointIds().SetId(2, 2)

aTriangleGrid = svtk.svtkUnstructuredGrid()
aTriangleGrid.Allocate(1, 1)
aTriangleGrid.InsertNextCell(aTriangle.GetCellType(), aTriangle.GetPointIds())
aTriangleGrid.SetPoints(trianglePoints)
aTriangleGrid.GetPointData().SetScalars(triangleScalars)

triangleContours = svtk.svtkContourFilter()
triangleContours.SetInputData(aTriangleGrid)
triangleContours.SetValue(0, .5)

aTriangleContourMapper = svtk.svtkDataSetMapper()
aTriangleContourMapper.SetInputConnection(triangleContours.GetOutputPort())
aTriangleContourMapper.ScalarVisibilityOff()

aTriangleContourActor = svtk.svtkActor()
aTriangleContourActor.SetMapper(aTriangleContourMapper)

aTriangleMapper = svtk.svtkDataSetMapper()
aTriangleMapper.SetInputData(aTriangleGrid)
aTriangleMapper.ScalarVisibilityOff()

aTriangleActor = svtk.svtkActor()
aTriangleActor.SetMapper(aTriangleMapper)
aTriangleActor.GetProperty().BackfaceCullingOn()
aTriangleActor.GetProperty().SetRepresentationToWireframe()

# Polygon

polygonPoints = svtk.svtkPoints()
polygonPoints.SetNumberOfPoints(4)
polygonPoints.InsertPoint(0, 0, 0, 0)
polygonPoints.InsertPoint(1, 1, 0, 0)
polygonPoints.InsertPoint(2, 1, 1, 0)
polygonPoints.InsertPoint(3, 0, 1, 0)

polygonScalars = svtk.svtkFloatArray()
polygonScalars.SetNumberOfTuples(4)
polygonScalars.InsertValue(0, 1)
polygonScalars.InsertValue(1, 0)
polygonScalars.InsertValue(2, 0)
polygonScalars.InsertValue(3, 0)

aPolygon = svtk.svtkPolygon()
aPolygon.GetPointIds().SetNumberOfIds(4)
aPolygon.GetPointIds().SetId(0, 0)
aPolygon.GetPointIds().SetId(1, 1)
aPolygon.GetPointIds().SetId(2, 2)
aPolygon.GetPointIds().SetId(3, 3)

aPolygonGrid = svtk.svtkUnstructuredGrid()
aPolygonGrid.Allocate(1, 1)
aPolygonGrid.InsertNextCell(aPolygon.GetCellType(), aPolygon.GetPointIds())
aPolygonGrid.SetPoints(polygonPoints)
aPolygonGrid.GetPointData().SetScalars(polygonScalars)

polygonContours = svtk.svtkContourFilter()
polygonContours.SetInputData(aPolygonGrid)
polygonContours.SetValue(0, .5)

aPolygonContourMapper = svtk.svtkDataSetMapper()
aPolygonContourMapper.SetInputConnection(polygonContours.GetOutputPort())
aPolygonContourMapper.ScalarVisibilityOff()

aPolygonMapper = svtk.svtkDataSetMapper()
aPolygonMapper.SetInputData(aPolygonGrid)
aPolygonMapper.ScalarVisibilityOff()

aPolygonContourActor = svtk.svtkActor()
aPolygonContourActor.SetMapper(aPolygonContourMapper)

aPolygonActor = svtk.svtkActor()
aPolygonActor.SetMapper(aPolygonMapper)
aPolygonActor.GetProperty().BackfaceCullingOn()
aPolygonActor.GetProperty().SetRepresentationToWireframe()

# Triangle strip

triangleStripPoints = svtk.svtkPoints()
triangleStripPoints.SetNumberOfPoints(5)
triangleStripPoints.InsertPoint(0, 0, 1, 0)
triangleStripPoints.InsertPoint(1, 0, 0, 0)
triangleStripPoints.InsertPoint(2, 1, 1, 0)
triangleStripPoints.InsertPoint(3, 1, 0, 0)
triangleStripPoints.InsertPoint(4, 2, 1, 0)

triangleStripScalars = svtk.svtkFloatArray()
triangleStripScalars.SetNumberOfTuples(5)
triangleStripScalars.InsertValue(0, 1)
triangleStripScalars.InsertValue(1, 0)
triangleStripScalars.InsertValue(2, 0)
triangleStripScalars.InsertValue(3, 0)
triangleStripScalars.InsertValue(4, 0)

aTriangleStrip = svtk.svtkTriangleStrip()
aTriangleStrip.GetPointIds().SetNumberOfIds(5)
aTriangleStrip.GetPointIds().SetId(0, 0)
aTriangleStrip.GetPointIds().SetId(1, 1)
aTriangleStrip.GetPointIds().SetId(2, 2)
aTriangleStrip.GetPointIds().SetId(3, 3)
aTriangleStrip.GetPointIds().SetId(4, 4)

aTriangleStripGrid = svtk.svtkUnstructuredGrid()
aTriangleStripGrid.Allocate(1, 1)
aTriangleStripGrid.InsertNextCell(aTriangleStrip.GetCellType(), aTriangleStrip.GetPointIds())
aTriangleStripGrid.SetPoints(triangleStripPoints)
aTriangleStripGrid.GetPointData().SetScalars(triangleStripScalars)

aTriangleStripMapper = svtk.svtkDataSetMapper()
aTriangleStripMapper.SetInputData(aTriangleStripGrid)
aTriangleStripMapper.ScalarVisibilityOff()

triangleStripContours = svtk.svtkContourFilter()
triangleStripContours.SetInputData(aTriangleStripGrid)
triangleStripContours.SetValue(0, .5)

aTriangleStripContourMapper = svtk.svtkDataSetMapper()
aTriangleStripContourMapper.SetInputConnection(triangleStripContours.GetOutputPort())
aTriangleStripContourMapper.ScalarVisibilityOff()

aTriangleStripContourActor = svtk.svtkActor()
aTriangleStripContourActor.SetMapper(aTriangleStripContourMapper)

aTriangleStripActor = svtk.svtkActor()
aTriangleStripActor.SetMapper(aTriangleStripMapper)
aTriangleStripActor.GetProperty().BackfaceCullingOn()
aTriangleStripActor.GetProperty().SetRepresentationToWireframe()

# Line

linePoints = svtk.svtkPoints()
linePoints.SetNumberOfPoints(2)
linePoints.InsertPoint(0, 0, 0, 0)
linePoints.InsertPoint(1, 1, 1, 0)
lineScalars = svtk.svtkFloatArray()
lineScalars.SetNumberOfTuples(2)
lineScalars.InsertValue(0, 1)
lineScalars.InsertValue(1, 0)

aLine = svtk.svtkLine()
aLine.GetPointIds().SetId(0, 0)
aLine.GetPointIds().SetId(1, 1)
aLineGrid = svtk.svtkUnstructuredGrid()

aLineGrid.Allocate(1, 1)
aLineGrid.InsertNextCell(aLine.GetCellType(), aLine.GetPointIds())
aLineGrid.SetPoints(linePoints)
aLineGrid.GetPointData().SetScalars(lineScalars)

lineContours = svtk.svtkContourFilter()
lineContours.SetInputData(aLineGrid)
lineContours.SetValue(0, .5)

aLineContourMapper = svtk.svtkDataSetMapper()
aLineContourMapper.SetInputConnection(lineContours.GetOutputPort())
aLineContourMapper.ScalarVisibilityOff()

aLineContourActor = svtk.svtkActor()
aLineContourActor.SetMapper(aLineContourMapper)

aLineMapper = svtk.svtkDataSetMapper()
aLineMapper.SetInputData(aLineGrid)
aLineMapper.ScalarVisibilityOff()

aLineActor = svtk.svtkActor()
aLineActor.SetMapper(aLineMapper)
aLineActor.GetProperty().BackfaceCullingOn()
aLineActor.GetProperty().SetRepresentationToWireframe()

# Polyline

polyLinePoints = svtk.svtkPoints()
polyLinePoints.SetNumberOfPoints(3)
polyLinePoints.InsertPoint(0, 0, 0, 0)
polyLinePoints.InsertPoint(1, 1, 1, 0)
polyLinePoints.InsertPoint(2, 1, 0, 0)

polyLineScalars = svtk.svtkFloatArray()
polyLineScalars.SetNumberOfTuples(3)
polyLineScalars.InsertValue(0, 1)
polyLineScalars.InsertValue(1, 0)
polyLineScalars.InsertValue(2, 0)

aPolyLine = svtk.svtkPolyLine()
aPolyLine.GetPointIds().SetNumberOfIds(3)
aPolyLine.GetPointIds().SetId(0, 0)
aPolyLine.GetPointIds().SetId(1, 1)
aPolyLine.GetPointIds().SetId(2, 2)

aPolyLineGrid = svtk.svtkUnstructuredGrid()
aPolyLineGrid.Allocate(1, 1)
aPolyLineGrid.InsertNextCell(aPolyLine.GetCellType(), aPolyLine.GetPointIds())
aPolyLineGrid.SetPoints(polyLinePoints)
aPolyLineGrid.GetPointData().SetScalars(polyLineScalars)

polyLineContours = svtk.svtkContourFilter()
polyLineContours.SetInputData(aPolyLineGrid)
polyLineContours.SetValue(0, .5)

aPolyLineContourMapper = svtk.svtkDataSetMapper()
aPolyLineContourMapper.SetInputConnection(polyLineContours.GetOutputPort())
aPolyLineContourMapper.ScalarVisibilityOff()

aPolyLineContourActor = svtk.svtkActor()
aPolyLineContourActor.SetMapper(aPolyLineContourMapper)

aPolyLineMapper = svtk.svtkDataSetMapper()
aPolyLineMapper.SetInputData(aPolyLineGrid)
aPolyLineMapper.ScalarVisibilityOff()

aPolyLineActor = svtk.svtkActor()
aPolyLineActor.SetMapper(aPolyLineMapper)
aPolyLineActor.GetProperty().BackfaceCullingOn()
aPolyLineActor.GetProperty().SetRepresentationToWireframe()

# Vertex

vertexPoints = svtk.svtkPoints()
vertexPoints.SetNumberOfPoints(1)
vertexPoints.InsertPoint(0, 0, 0, 0)

vertexScalars = svtk.svtkFloatArray()
vertexScalars.SetNumberOfTuples(1)
vertexScalars.InsertValue(0, 1)

aVertex = svtk.svtkVertex()
aVertex.GetPointIds().SetId(0, 0)

aVertexGrid = svtk.svtkUnstructuredGrid()
aVertexGrid.Allocate(1, 1)
aVertexGrid.InsertNextCell(aVertex.GetCellType(), aVertex.GetPointIds())
aVertexGrid.SetPoints(vertexPoints)
aVertexGrid.GetPointData().SetScalars(vertexScalars)

vertexContours = svtk.svtkContourFilter()
vertexContours.SetInputData(aVertexGrid)
vertexContours.SetValue(0, 1)

aVertexContourMapper = svtk.svtkDataSetMapper()
aVertexContourMapper.SetInputConnection(vertexContours.GetOutputPort())
aVertexContourMapper.ScalarVisibilityOff()

aVertexContourActor = svtk.svtkActor()
aVertexContourActor.SetMapper(aVertexContourMapper)
aVertexContourActor.GetProperty().SetRepresentationToWireframe()

aVertexMapper = svtk.svtkDataSetMapper()
aVertexMapper.SetInputData(aVertexGrid)
aVertexMapper.ScalarVisibilityOff()

aVertexActor = svtk.svtkActor()
aVertexActor.SetMapper(aVertexMapper)
aVertexActor.GetProperty().BackfaceCullingOn()

# Poly Vertex

polyVertexPoints = svtk.svtkPoints()
polyVertexPoints.SetNumberOfPoints(3)
polyVertexPoints.InsertPoint(0, 0, 0, 0)
polyVertexPoints.InsertPoint(1, 1, 0, 0)
polyVertexPoints.InsertPoint(2, 1, 1, 0)

polyVertexScalars = svtk.svtkFloatArray()
polyVertexScalars.SetNumberOfTuples(3)
polyVertexScalars.InsertValue(0, 1)
polyVertexScalars.InsertValue(1, 0)
polyVertexScalars.InsertValue(2, 0)

aPolyVertex = svtk.svtkPolyVertex()
aPolyVertex.GetPointIds().SetNumberOfIds(3)
aPolyVertex.GetPointIds().SetId(0, 0)
aPolyVertex.GetPointIds().SetId(1, 1)
aPolyVertex.GetPointIds().SetId(2, 2)

aPolyVertexGrid = svtk.svtkUnstructuredGrid()
aPolyVertexGrid.Allocate(1, 1)
aPolyVertexGrid.InsertNextCell(aPolyVertex.GetCellType(), aPolyVertex.GetPointIds())
aPolyVertexGrid.SetPoints(polyVertexPoints)
aPolyVertexGrid.GetPointData().SetScalars(polyVertexScalars)

polyVertexContours = svtk.svtkContourFilter()
polyVertexContours.SetInputData(aPolyVertexGrid)
polyVertexContours.SetValue(0, 0)

aPolyVertexContourMapper = svtk.svtkDataSetMapper()
aPolyVertexContourMapper.SetInputConnection(polyVertexContours.GetOutputPort())
aPolyVertexContourMapper.ScalarVisibilityOff()

aPolyVertexContourActor = svtk.svtkActor()
aPolyVertexContourActor.SetMapper(aPolyVertexContourMapper)
aPolyVertexContourActor.GetProperty().SetRepresentationToWireframe()

aPolyVertexMapper = svtk.svtkDataSetMapper()
aPolyVertexMapper.SetInputData(aPolyVertexGrid)
aPolyVertexMapper.ScalarVisibilityOff()

aPolyVertexActor = svtk.svtkActor()
aPolyVertexActor.SetMapper(aPolyVertexMapper)

# Pentagonal prism

pentaPoints = svtk.svtkPoints()
pentaPoints.SetNumberOfPoints(10)
pentaPoints.InsertPoint(0, 0.25, 0.0, 0.0)
pentaPoints.InsertPoint(1, 0.75, 0.0, 0.0)
pentaPoints.InsertPoint(2, 1.0, 0.5, 0.0)
pentaPoints.InsertPoint(3, 0.5, 1.0, 0.0)
pentaPoints.InsertPoint(4, 0.0, 0.5, 0.0)
pentaPoints.InsertPoint(5, 0.25, 0.0, 1.0)
pentaPoints.InsertPoint(6, 0.75, 0.0, 1.0)
pentaPoints.InsertPoint(7, 1.0, 0.5, 1.0)
pentaPoints.InsertPoint(8, 0.5, 1.0, 1.0)
pentaPoints.InsertPoint(9, 0.0, 0.5, 1.0)

pentaScalars = svtk.svtkFloatArray()
pentaScalars.SetNumberOfTuples(10)
pentaScalars.InsertValue(0, 1)
pentaScalars.InsertValue(1, 1)
pentaScalars.InsertValue(2, 1)
pentaScalars.InsertValue(3, 1)
pentaScalars.InsertValue(4, 1)
pentaScalars.InsertValue(5, 0)
pentaScalars.InsertValue(6, 0)
pentaScalars.InsertValue(7, 0)
pentaScalars.InsertValue(8, 0)
pentaScalars.InsertValue(9, 0)

aPenta = svtk.svtkPentagonalPrism()
aPenta.GetPointIds().SetId(0, 0)
aPenta.GetPointIds().SetId(1, 1)
aPenta.GetPointIds().SetId(2, 2)
aPenta.GetPointIds().SetId(3, 3)
aPenta.GetPointIds().SetId(4, 4)
aPenta.GetPointIds().SetId(5, 5)
aPenta.GetPointIds().SetId(6, 6)
aPenta.GetPointIds().SetId(7, 7)
aPenta.GetPointIds().SetId(8, 8)
aPenta.GetPointIds().SetId(9, 9)

aPentaGrid = svtk.svtkUnstructuredGrid()
aPentaGrid.Allocate(1, 1)
aPentaGrid.InsertNextCell(aPenta.GetCellType(), aPenta.GetPointIds())
aPentaGrid.SetPoints(pentaPoints)
aPentaGrid.GetPointData().SetScalars(pentaScalars)

pentaContours = svtk.svtkContourFilter()
pentaContours.SetInputData(aPentaGrid)
pentaContours.SetValue(0, .5)

aPentaContourMapper = svtk.svtkDataSetMapper()
aPentaContourMapper.SetInputConnection(pentaContours.GetOutputPort())
aPentaContourMapper.ScalarVisibilityOff()

aPentaMapper = svtk.svtkDataSetMapper()
aPentaMapper.SetInputData(aPentaGrid)
aPentaMapper.ScalarVisibilityOff()

aPentaActor = svtk.svtkActor()
aPentaActor.SetMapper(aPentaMapper)
aPentaActor.GetProperty().BackfaceCullingOn()
aPentaActor.GetProperty().SetRepresentationToWireframe()

aPentaContourActor = svtk.svtkActor()
aPentaContourActor.SetMapper(aPentaContourMapper)
aPentaContourActor.GetProperty().BackfaceCullingOn()

# Hexagonal prism

hexaPoints = svtk.svtkPoints()
hexaPoints.SetNumberOfPoints(12)
hexaPoints.InsertPoint(0, 0.0, 0.0, 0.0)
hexaPoints.InsertPoint(1, 0.5, 0.0, 0.0)
hexaPoints.InsertPoint(2, 1.0, 0.5, 0.0)
hexaPoints.InsertPoint(3, 1.0, 1.0, 0.0)
hexaPoints.InsertPoint(4, 0.5, 1.0, 0.0)
hexaPoints.InsertPoint(5, 0.0, 0.5, 0.0)
hexaPoints.InsertPoint(6, 0.0, 0.0, 1.0)
hexaPoints.InsertPoint(7, 0.5, 0.0, 1.0)
hexaPoints.InsertPoint(8, 1.0, 0.5, 1.0)
hexaPoints.InsertPoint(9, 1.0, 1.0, 1.0)
hexaPoints.InsertPoint(10, 0.5, 1.0, 1.0)
hexaPoints.InsertPoint(11, 0.0, 0.5, 1.0)

hexaScalars = svtk.svtkFloatArray()
hexaScalars.SetNumberOfTuples(12)
hexaScalars.InsertValue(0, 1)
hexaScalars.InsertValue(1, 1)
hexaScalars.InsertValue(2, 1)
hexaScalars.InsertValue(3, 1)
hexaScalars.InsertValue(4, 1)
hexaScalars.InsertValue(5, 1)
hexaScalars.InsertValue(6, 0)
hexaScalars.InsertValue(7, 0)
hexaScalars.InsertValue(8, 0)
hexaScalars.InsertValue(9, 0)
hexaScalars.InsertValue(10, 0)
hexaScalars.InsertValue(11, 0)

aHexa = svtk.svtkHexagonalPrism()
aHexa.GetPointIds().SetId(0, 0)
aHexa.GetPointIds().SetId(1, 1)
aHexa.GetPointIds().SetId(2, 2)
aHexa.GetPointIds().SetId(3, 3)
aHexa.GetPointIds().SetId(4, 4)
aHexa.GetPointIds().SetId(5, 5)
aHexa.GetPointIds().SetId(6, 6)
aHexa.GetPointIds().SetId(7, 7)
aHexa.GetPointIds().SetId(8, 8)
aHexa.GetPointIds().SetId(9, 9)
aHexa.GetPointIds().SetId(10, 10)
aHexa.GetPointIds().SetId(11, 11)

aHexaGrid = svtk.svtkUnstructuredGrid()
aHexaGrid.Allocate(1, 1)
aHexaGrid.InsertNextCell(aHexa.GetCellType(), aHexa.GetPointIds())
aHexaGrid.SetPoints(hexaPoints)
aHexaGrid.GetPointData().SetScalars(hexaScalars)

hexaContours = svtk.svtkContourFilter()
hexaContours.SetInputData(aHexaGrid)
hexaContours.SetValue(0, .5)

aHexaContourMapper = svtk.svtkDataSetMapper()
aHexaContourMapper.SetInputConnection(hexaContours.GetOutputPort())
aHexaContourMapper.ScalarVisibilityOff()

aHexaMapper = svtk.svtkDataSetMapper()
aHexaMapper.SetInputData(aHexaGrid)
aHexaMapper.ScalarVisibilityOff()

aHexaActor = svtk.svtkActor()
aHexaActor.SetMapper(aHexaMapper)
aHexaActor.GetProperty().BackfaceCullingOn()
aHexaActor.GetProperty().SetRepresentationToWireframe()

aHexaContourActor = svtk.svtkActor()
aHexaContourActor.SetMapper(aHexaContourMapper)
aHexaContourActor.GetProperty().BackfaceCullingOn()

ren1.SetBackground(.1, .2, .3)
renWin.SetSize(400, 400)

ren1.AddActor(aVoxelActor)
aVoxelActor.GetProperty().SetDiffuseColor(1, 0, 0)

ren1.AddActor(aVoxelContourActor)
aVoxelContourActor.GetProperty().SetDiffuseColor(1, 0, 0)

ren1.AddActor(aHexahedronActor)
aHexahedronActor.GetProperty().SetDiffuseColor(1, 1, 0)

ren1.AddActor(aHexahedronContourActor)
aHexahedronContourActor.GetProperty().SetDiffuseColor(1, 1, 0)

ren1.AddActor(aTetraActor)
aTetraActor.GetProperty().SetDiffuseColor(0, 1, 0)

ren1.AddActor(aTetraContourActor)
aTetraContourActor.GetProperty().SetDiffuseColor(0, 1, 0)

ren1.AddActor(aWedgeActor)
aWedgeActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aWedgeContourActor)
aWedgeContourActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aPyramidActor)
aPyramidActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aPyramidContourActor)
aPyramidContourActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aPixelActor)
aPixelActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aPixelContourActor)
aPixelContourActor.GetProperty().SetDiffuseColor(0, 1, 1)

ren1.AddActor(aQuadActor)
aQuadActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aQuadContourActor)
aQuadContourActor.GetProperty().SetDiffuseColor(1, 0, 1)

ren1.AddActor(aTriangleActor)
aTriangleActor.GetProperty().SetDiffuseColor(.3, 1, .5)

ren1.AddActor(aTriangleContourActor)
aTriangleContourActor.GetProperty().SetDiffuseColor(.3, 1, .5)

ren1.AddActor(aPolygonActor)
aPolygonActor.GetProperty().SetDiffuseColor(1, .4, .5)

ren1.AddActor(aPolygonContourActor)
aPolygonContourActor.GetProperty().SetDiffuseColor(1, .4, .5)

ren1.AddActor(aTriangleStripActor)
aTriangleStripActor.GetProperty().SetDiffuseColor(.3, .7, 1)

ren1.AddActor(aTriangleStripContourActor)
aTriangleStripContourActor.GetProperty().SetDiffuseColor(.3, .7, 1)

ren1.AddActor(aLineActor)
aLineActor.GetProperty().SetDiffuseColor(.2, 1, 1)

ren1.AddActor(aLineContourActor)
aLineContourActor.GetProperty().SetDiffuseColor(.2, 1, 1)

ren1.AddActor(aPolyLineActor)
aPolyLineActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyLineContourActor)
aPolyLineContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aVertexActor)
aVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aVertexContourActor)
aVertexContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyVertexActor)
aPolyVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPolyVertexContourActor)
aPolyVertexContourActor.GetProperty().SetDiffuseColor(1, 1, 1)

ren1.AddActor(aPentaActor)
aPentaActor.GetProperty().SetDiffuseColor(.2, .4, .7)

ren1.AddActor(aPentaContourActor)
aPentaContourActor.GetProperty().SetDiffuseColor(.2, .4, .7)

ren1.AddActor(aHexaActor)
aHexaActor.GetProperty().SetDiffuseColor(.7, .5, 1)

ren1.AddActor(aHexaContourActor)
aHexaContourActor.GetProperty().SetDiffuseColor(.7, .5, 1)

# places everyone!!

aVoxelContourActor.AddPosition(0, 0, 0)
aVoxelContourActor.AddPosition(0, 2, 0)
aHexahedronContourActor.AddPosition(2, 0, 0)
aHexahedronContourActor.AddPosition(0, 2, 0)
aHexahedronActor.AddPosition(2, 0, 0)
aTetraContourActor.AddPosition(4, 0, 0)
aTetraContourActor.AddPosition(0, 2, 0)
aTetraActor.AddPosition(4, 0, 0)
aWedgeContourActor.AddPosition(6, 0, 0)
aWedgeContourActor.AddPosition(0, 2, 0)
aWedgeActor.AddPosition(6, 0, 0)
aPyramidContourActor.AddPosition(8, 0, 0)
aPyramidContourActor.AddPosition(0, 2, 0)
aPyramidActor.AddPosition(8, 0, 0)

aPixelContourActor.AddPosition(0, 4, 0)
aPixelContourActor.AddPosition(0, 2, 0)
aPixelActor.AddPosition(0, 4, 0)
aQuadContourActor.AddPosition(2, 4, 0)
aQuadContourActor.AddPosition(0, 2, 0)
aQuadActor.AddPosition(2, 4, 0)
aTriangleContourActor.AddPosition(4, 4, 0)
aTriangleContourActor.AddPosition(0, 2, 0)
aTriangleActor.AddPosition(4, 4, 0)
aPolygonContourActor.AddPosition(6, 4, 0)
aPolygonContourActor.AddPosition(0, 2, 0)
aPolygonActor.AddPosition(6, 4, 0)
aTriangleStripContourActor.AddPosition(8, 4, 0)
aTriangleStripContourActor.AddPosition(0, 2, 0)
aTriangleStripActor.AddPosition(8, 4, 0)

aLineContourActor.AddPosition(0, 8, 0)
aLineContourActor.AddPosition(0, 2, 0)
aLineActor.AddPosition(0, 8, 0)
aPolyLineContourActor.AddPosition(2, 8, 0)
aPolyLineContourActor.AddPosition(0, 2, 0)
aPolyLineActor.AddPosition(2, 8, 0)

aVertexContourActor.AddPosition(0, 12, 0)
aVertexContourActor.AddPosition(0, 2, 0)
aVertexActor.AddPosition(0, 12, 0)
aPolyVertexContourActor.AddPosition(2, 12, 0)
aPolyVertexContourActor.AddPosition(0, 2, 0)
aPolyVertexActor.AddPosition(2, 12, 0)

aPentaContourActor.AddPosition(4, 8, 0)
aPentaContourActor.AddPosition(0, 2, 0)
aPentaActor.AddPosition(4, 8, 0)
aHexaContourActor.AddPosition(6, 8, 0)
aHexaContourActor.AddPosition(0, 2, 0)
aHexaActor.AddPosition(6, 8, 0)

[base, back, left] = backdrop.BuildBackdrop(-1, 11, -1, 16, -1, 2, .1)

ren1.AddActor(base)
base.GetProperty().SetDiffuseColor(.2, .2, .2)
ren1.AddActor(left)
left.GetProperty().SetDiffuseColor(.2, .2, .2)
ren1.AddActor(back)
back.GetProperty().SetDiffuseColor(.2, .2, .2)

ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
