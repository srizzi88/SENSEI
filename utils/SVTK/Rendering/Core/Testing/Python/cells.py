#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import os
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot, svtkGetTempDir
SVTK_DATA_ROOT = svtkGetDataRoot()
SVTK_TEMP_DIR = svtkGetTempDir()

class cells(svtk.test.Testing.svtkTest):

    def testCells(self):

        # Demonstrates all cell types
        #
        # NOTE: the use of NewInstance/DeepCopy is included to increase
        # regression coverage.  It is not required in most applications.

        ren = svtk.svtkRenderer()
        # turn off all cullers
        ren.GetCullers().RemoveAllItems()

        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        renWin.SetSize(300, 150)
        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);

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

        aVoxel = svtk.svtkVoxel()
        aVoxel.GetPointIds().SetId(0, 0)
        aVoxel.GetPointIds().SetId(1, 1)
        aVoxel.GetPointIds().SetId(2, 2)
        aVoxel.GetPointIds().SetId(3, 3)
        aVoxel.GetPointIds().SetId(4, 4)
        aVoxel.GetPointIds().SetId(5, 5)
        aVoxel.GetPointIds().SetId(6, 6)
        aVoxel.GetPointIds().SetId(7, 7)

        bVoxel = aVoxel.NewInstance()
        bVoxel.DeepCopy(aVoxel)

        aVoxelGrid = svtk.svtkUnstructuredGrid()
        aVoxelGrid.Allocate(1, 1)
        aVoxelGrid.InsertNextCell(aVoxel.GetCellType(), aVoxel.GetPointIds())
        aVoxelGrid.SetPoints(voxelPoints)

        aVoxelMapper = svtk.svtkDataSetMapper()
        aVoxelMapper.SetInputData(aVoxelGrid)

        aVoxelActor = svtk.svtkActor()
        aVoxelActor.SetMapper(aVoxelMapper)
        aVoxelActor.GetProperty().BackfaceCullingOn()

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

        aHexahedron = svtk.svtkHexahedron()
        aHexahedron.GetPointIds().SetId(0, 0)
        aHexahedron.GetPointIds().SetId(1, 1)
        aHexahedron.GetPointIds().SetId(2, 2)
        aHexahedron.GetPointIds().SetId(3, 3)
        aHexahedron.GetPointIds().SetId(4, 4)
        aHexahedron.GetPointIds().SetId(5, 5)
        aHexahedron.GetPointIds().SetId(6, 6)
        aHexahedron.GetPointIds().SetId(7, 7)

        bHexahedron = aHexahedron.NewInstance()
        bHexahedron.DeepCopy(aHexahedron)

        aHexahedronGrid = svtk.svtkUnstructuredGrid()
        aHexahedronGrid.Allocate(1, 1)
        aHexahedronGrid.InsertNextCell(aHexahedron.GetCellType(), aHexahedron.GetPointIds())
        aHexahedronGrid.SetPoints(hexahedronPoints)

        aHexahedronMapper = svtk.svtkDataSetMapper()
        aHexahedronMapper.SetInputData(aHexahedronGrid)

        aHexahedronActor = svtk.svtkActor()
        aHexahedronActor.SetMapper(aHexahedronMapper)
        aHexahedronActor.AddPosition(2, 0, 0)
        aHexahedronActor.GetProperty().BackfaceCullingOn()

        # Tetra

        tetraPoints = svtk.svtkPoints()
        tetraPoints.SetNumberOfPoints(4)
        tetraPoints.InsertPoint(0, 0, 0, 0)
        tetraPoints.InsertPoint(1, 1, 0, 0)
        tetraPoints.InsertPoint(2, .5, 1, 0)
        tetraPoints.InsertPoint(3, .5, .5, 1)

        aTetra = svtk.svtkTetra()
        aTetra.GetPointIds().SetId(0, 0)
        aTetra.GetPointIds().SetId(1, 1)
        aTetra.GetPointIds().SetId(2, 2)
        aTetra.GetPointIds().SetId(3, 3)

        bTetra = aTetra.NewInstance()
        bTetra.DeepCopy(aTetra)

        aTetraGrid = svtk.svtkUnstructuredGrid()
        aTetraGrid.Allocate(1, 1)
        aTetraGrid.InsertNextCell(aTetra.GetCellType(), aTetra.GetPointIds())
        aTetraGrid.SetPoints(tetraPoints)

        aTetraCopy = svtk.svtkUnstructuredGrid()
        aTetraCopy.ShallowCopy(aTetraGrid)

        aTetraMapper = svtk.svtkDataSetMapper()
        aTetraMapper.SetInputData(aTetraCopy)

        aTetraActor = svtk.svtkActor()
        aTetraActor.SetMapper(aTetraMapper)
        aTetraActor.AddPosition(4, 0, 0)
        aTetraActor.GetProperty().BackfaceCullingOn()

        # Wedge

        wedgePoints = svtk.svtkPoints()
        wedgePoints.SetNumberOfPoints(6)
        wedgePoints.InsertPoint(0, 0, 1, 0)
        wedgePoints.InsertPoint(1, 0, 0, 0)
        wedgePoints.InsertPoint(2, 0, .5, .5)
        wedgePoints.InsertPoint(3, 1, 1, 0)
        wedgePoints.InsertPoint(4, 1, 0, 0)
        wedgePoints.InsertPoint(5, 1, .5, .5)

        aWedge = svtk.svtkWedge()
        aWedge.GetPointIds().SetId(0, 0)
        aWedge.GetPointIds().SetId(1, 1)
        aWedge.GetPointIds().SetId(2, 2)
        aWedge.GetPointIds().SetId(3, 3)
        aWedge.GetPointIds().SetId(4, 4)
        aWedge.GetPointIds().SetId(5, 5)

        bWedge = aWedge.NewInstance()
        bWedge.DeepCopy(aWedge)

        aWedgeGrid = svtk.svtkUnstructuredGrid()
        aWedgeGrid.Allocate(1, 1)
        aWedgeGrid.InsertNextCell(aWedge.GetCellType(), aWedge.GetPointIds())
        aWedgeGrid.SetPoints(wedgePoints)

        aWedgeCopy = svtk.svtkUnstructuredGrid()
        aWedgeCopy.DeepCopy(aWedgeGrid)

        aWedgeMapper = svtk.svtkDataSetMapper()
        aWedgeMapper.SetInputData(aWedgeCopy)

        aWedgeActor = svtk.svtkActor()
        aWedgeActor.SetMapper(aWedgeMapper)
        aWedgeActor.AddPosition(6, 0, 0)
        aWedgeActor.GetProperty().BackfaceCullingOn()

        # Pyramid

        pyramidPoints = svtk.svtkPoints()
        pyramidPoints.SetNumberOfPoints(5)
        pyramidPoints.InsertPoint(0, 0, 0, 0)
        pyramidPoints.InsertPoint(1, 1, 0, 0)
        pyramidPoints.InsertPoint(2, 1, 1, 0)
        pyramidPoints.InsertPoint(3, 0, 1, 0)
        pyramidPoints.InsertPoint(4, .5, .5, 1)

        aPyramid = svtk.svtkPyramid()
        aPyramid.GetPointIds().SetId(0, 0)
        aPyramid.GetPointIds().SetId(1, 1)
        aPyramid.GetPointIds().SetId(2, 2)
        aPyramid.GetPointIds().SetId(3, 3)
        aPyramid.GetPointIds().SetId(4, 4)

        bPyramid = aPyramid.NewInstance()
        bPyramid.DeepCopy(aPyramid)

        aPyramidGrid = svtk.svtkUnstructuredGrid()
        aPyramidGrid.Allocate(1, 1)
        aPyramidGrid.InsertNextCell(aPyramid.GetCellType(), aPyramid.GetPointIds())
        aPyramidGrid.SetPoints(pyramidPoints)

        aPyramidMapper = svtk.svtkDataSetMapper()
        aPyramidMapper.SetInputData(aPyramidGrid)

        aPyramidActor = svtk.svtkActor()
        aPyramidActor.SetMapper(aPyramidMapper)
        aPyramidActor.AddPosition(8, 0, 0)
        aPyramidActor.GetProperty().BackfaceCullingOn()

        # Pixel

        pixelPoints = svtk.svtkPoints()
        pixelPoints.SetNumberOfPoints(4)
        pixelPoints.InsertPoint(0, 0, 0, 0)
        pixelPoints.InsertPoint(1, 1, 0, 0)
        pixelPoints.InsertPoint(2, 0, 1, 0)
        pixelPoints.InsertPoint(3, 1, 1, 0)

        aPixel = svtk.svtkPixel()
        aPixel.GetPointIds().SetId(0, 0)
        aPixel.GetPointIds().SetId(1, 1)
        aPixel.GetPointIds().SetId(2, 2)
        aPixel.GetPointIds().SetId(3, 3)

        bPixel = aPixel.NewInstance()
        bPixel.DeepCopy(aPixel)

        aPixelGrid = svtk.svtkUnstructuredGrid()
        aPixelGrid.Allocate(1, 1)
        aPixelGrid.InsertNextCell(aPixel.GetCellType(), aPixel.GetPointIds())
        aPixelGrid.SetPoints(pixelPoints)

        aPixelMapper = svtk.svtkDataSetMapper()
        aPixelMapper.SetInputData(aPixelGrid)

        aPixelActor = svtk.svtkActor()
        aPixelActor.SetMapper(aPixelMapper)
        aPixelActor.AddPosition(0, 0, 2)
        aPixelActor.GetProperty().BackfaceCullingOn()

        # Quad

        quadPoints = svtk.svtkPoints()
        quadPoints.SetNumberOfPoints(4)
        quadPoints.InsertPoint(0, 0, 0, 0)
        quadPoints.InsertPoint(1, 1, 0, 0)
        quadPoints.InsertPoint(2, 1, 1, 0)
        quadPoints.InsertPoint(3, 0, 1, 0)

        aQuad = svtk.svtkQuad()
        aQuad.GetPointIds().SetId(0, 0)
        aQuad.GetPointIds().SetId(1, 1)
        aQuad.GetPointIds().SetId(2, 2)
        aQuad.GetPointIds().SetId(3, 3)

        bQuad = aQuad.NewInstance()
        bQuad.DeepCopy(aQuad)

        aQuadGrid = svtk.svtkUnstructuredGrid()
        aQuadGrid.Allocate(1, 1)
        aQuadGrid.InsertNextCell(aQuad.GetCellType(), aQuad.GetPointIds())
        aQuadGrid.SetPoints(quadPoints)

        aQuadMapper = svtk.svtkDataSetMapper()
        aQuadMapper.SetInputData(aQuadGrid)

        aQuadActor = svtk.svtkActor()
        aQuadActor.SetMapper(aQuadMapper)
        aQuadActor.AddPosition(2, 0, 2)
        aQuadActor.GetProperty().BackfaceCullingOn()

        # Triangle

        trianglePoints = svtk.svtkPoints()
        trianglePoints.SetNumberOfPoints(3)
        trianglePoints.InsertPoint(0, 0, 0, 0)
        trianglePoints.InsertPoint(1, 1, 0, 0)
        trianglePoints.InsertPoint(2, .5, .5, 0)

        triangleTCoords = svtk.svtkFloatArray()
        triangleTCoords.SetNumberOfComponents(2)
        triangleTCoords.SetNumberOfTuples(3)
        triangleTCoords.InsertTuple2(0, 1, 1)
        triangleTCoords.InsertTuple2(1, 2, 2)
        triangleTCoords.InsertTuple2(2, 3, 3)

        aTriangle = svtk.svtkTriangle()
        aTriangle.GetPointIds().SetId(0, 0)
        aTriangle.GetPointIds().SetId(1, 1)
        aTriangle.GetPointIds().SetId(2, 2)

        bTriangle = aTriangle.NewInstance()
        bTriangle.DeepCopy(aTriangle)

        aTriangleGrid = svtk.svtkUnstructuredGrid()
        aTriangleGrid.Allocate(1, 1)
        aTriangleGrid.InsertNextCell(aTriangle.GetCellType(), aTriangle.GetPointIds())
        aTriangleGrid.SetPoints(trianglePoints)
        aTriangleGrid.GetPointData().SetTCoords(triangleTCoords)

        aTriangleMapper = svtk.svtkDataSetMapper()
        aTriangleMapper.SetInputData(aTriangleGrid)

        aTriangleActor = svtk.svtkActor()
        aTriangleActor.SetMapper(aTriangleMapper)
        aTriangleActor.AddPosition(4, 0, 2)
        aTriangleActor.GetProperty().BackfaceCullingOn()

        # Polygon

        polygonPoints = svtk.svtkPoints()
        polygonPoints.SetNumberOfPoints(4)
        polygonPoints.InsertPoint(0, 0, 0, 0)
        polygonPoints.InsertPoint(1, 1, 0, 0)
        polygonPoints.InsertPoint(2, 1, 1, 0)
        polygonPoints.InsertPoint(3, 0, 1, 0)

        aPolygon = svtk.svtkPolygon()
        aPolygon.GetPointIds().SetNumberOfIds(4)
        aPolygon.GetPointIds().SetId(0, 0)
        aPolygon.GetPointIds().SetId(1, 1)
        aPolygon.GetPointIds().SetId(2, 2)
        aPolygon.GetPointIds().SetId(3, 3)

        bPolygon = aPolygon.NewInstance()
        bPolygon.DeepCopy(aPolygon)

        aPolygonGrid = svtk.svtkUnstructuredGrid()
        aPolygonGrid.Allocate(1, 1)
        aPolygonGrid.InsertNextCell(aPolygon.GetCellType(), aPolygon.GetPointIds())
        aPolygonGrid.SetPoints(polygonPoints)

        aPolygonMapper = svtk.svtkDataSetMapper()
        aPolygonMapper.SetInputData(aPolygonGrid)

        aPolygonActor = svtk.svtkActor()
        aPolygonActor.SetMapper(aPolygonMapper)
        aPolygonActor.AddPosition(6, 0, 2)
        aPolygonActor.GetProperty().BackfaceCullingOn()

        # Triangle Strip

        triangleStripPoints = svtk.svtkPoints()
        triangleStripPoints.SetNumberOfPoints(5)
        triangleStripPoints.InsertPoint(0, 0, 1, 0)
        triangleStripPoints.InsertPoint(1, 0, 0, 0)
        triangleStripPoints.InsertPoint(2, 1, 1, 0)
        triangleStripPoints.InsertPoint(3, 1, 0, 0)
        triangleStripPoints.InsertPoint(4, 2, 1, 0)

        triangleStripTCoords = svtk.svtkFloatArray()
        triangleStripTCoords.SetNumberOfComponents(2)
        triangleStripTCoords.SetNumberOfTuples(3)
        triangleStripTCoords.InsertTuple2(0, 1, 1)
        triangleStripTCoords.InsertTuple2(1, 2, 2)
        triangleStripTCoords.InsertTuple2(2, 3, 3)
        triangleStripTCoords.InsertTuple2(3, 4, 4)
        triangleStripTCoords.InsertTuple2(4, 5, 5)

        aTriangleStrip = svtk.svtkTriangleStrip()
        aTriangleStrip.GetPointIds().SetNumberOfIds(5)
        aTriangleStrip.GetPointIds().SetId(0, 0)
        aTriangleStrip.GetPointIds().SetId(1, 1)
        aTriangleStrip.GetPointIds().SetId(2, 2)
        aTriangleStrip.GetPointIds().SetId(3, 3)
        aTriangleStrip.GetPointIds().SetId(4, 4)

        bTriangleStrip = aTriangleStrip.NewInstance()
        bTriangleStrip.DeepCopy(aTriangleStrip)

        aTriangleStripGrid = svtk.svtkUnstructuredGrid()
        aTriangleStripGrid.Allocate(1, 1)
        aTriangleStripGrid.InsertNextCell(aTriangleStrip.GetCellType(), aTriangleStrip.GetPointIds())
        aTriangleStripGrid.SetPoints(triangleStripPoints)
        aTriangleStripGrid.GetPointData().SetTCoords(triangleStripTCoords)

        aTriangleStripMapper = svtk.svtkDataSetMapper()
        aTriangleStripMapper.SetInputData(aTriangleStripGrid)

        aTriangleStripActor = svtk.svtkActor()
        aTriangleStripActor.SetMapper(aTriangleStripMapper)
        aTriangleStripActor.AddPosition(8, 0, 2)
        aTriangleStripActor.GetProperty().BackfaceCullingOn()

        # Line

        linePoints = svtk.svtkPoints()
        linePoints.SetNumberOfPoints(2)
        linePoints.InsertPoint(0, 0, 0, 0)
        linePoints.InsertPoint(1, 1, 1, 0)

        aLine = svtk.svtkLine()
        aLine.GetPointIds().SetId(0, 0)
        aLine.GetPointIds().SetId(1, 1)

        bLine = aLine.NewInstance()
        bLine.DeepCopy(aLine)

        aLineGrid = svtk.svtkUnstructuredGrid()
        aLineGrid.Allocate(1, 1)
        aLineGrid.InsertNextCell(aLine.GetCellType(), aLine.GetPointIds())
        aLineGrid.SetPoints(linePoints)

        aLineMapper = svtk.svtkDataSetMapper()
        aLineMapper.SetInputData(aLineGrid)

        aLineActor = svtk.svtkActor()
        aLineActor.SetMapper(aLineMapper)
        aLineActor.AddPosition(0, 0, 4)
        aLineActor.GetProperty().BackfaceCullingOn()

        # Poly line

        polyLinePoints = svtk.svtkPoints()
        polyLinePoints.SetNumberOfPoints(3)
        polyLinePoints.InsertPoint(0, 0, 0, 0)
        polyLinePoints.InsertPoint(1, 1, 1, 0)
        polyLinePoints.InsertPoint(2, 1, 0, 0)

        aPolyLine = svtk.svtkPolyLine()
        aPolyLine.GetPointIds().SetNumberOfIds(3)
        aPolyLine.GetPointIds().SetId(0, 0)
        aPolyLine.GetPointIds().SetId(1, 1)
        aPolyLine.GetPointIds().SetId(2, 2)

        bPolyLine = aPolyLine.NewInstance()
        bPolyLine.DeepCopy(aPolyLine)

        aPolyLineGrid = svtk.svtkUnstructuredGrid()
        aPolyLineGrid.Allocate(1, 1)
        aPolyLineGrid.InsertNextCell(aPolyLine.GetCellType(), aPolyLine.GetPointIds())
        aPolyLineGrid.SetPoints(polyLinePoints)

        aPolyLineMapper = svtk.svtkDataSetMapper()
        aPolyLineMapper.SetInputData(aPolyLineGrid)

        aPolyLineActor = svtk.svtkActor()
        aPolyLineActor.SetMapper(aPolyLineMapper)
        aPolyLineActor.AddPosition(2, 0, 4)
        aPolyLineActor.GetProperty().BackfaceCullingOn()

        # Vertex

        vertexPoints = svtk.svtkPoints()
        vertexPoints.SetNumberOfPoints(1)
        vertexPoints.InsertPoint(0, 0, 0, 0)

        aVertex = svtk.svtkVertex()
        aVertex.GetPointIds().SetId(0, 0)

        bVertex = aVertex.NewInstance()
        bVertex.DeepCopy(aVertex)

        aVertexGrid = svtk.svtkUnstructuredGrid()
        aVertexGrid.Allocate(1, 1)
        aVertexGrid.InsertNextCell(aVertex.GetCellType(), aVertex.GetPointIds())
        aVertexGrid.SetPoints(vertexPoints)

        aVertexMapper = svtk.svtkDataSetMapper()
        aVertexMapper.SetInputData(aVertexGrid)

        aVertexActor = svtk.svtkActor()
        aVertexActor.SetMapper(aVertexMapper)
        aVertexActor.AddPosition(0, 0, 6)
        aVertexActor.GetProperty().BackfaceCullingOn()

        # Poly Vertex

        polyVertexPoints = svtk.svtkPoints()
        polyVertexPoints.SetNumberOfPoints(3)
        polyVertexPoints.InsertPoint(0, 0, 0, 0)
        polyVertexPoints.InsertPoint(1, 1, 0, 0)
        polyVertexPoints.InsertPoint(2, 1, 1, 0)

        aPolyVertex = svtk.svtkPolyVertex()
        aPolyVertex.GetPointIds().SetNumberOfIds(3)
        aPolyVertex.GetPointIds().SetId(0, 0)
        aPolyVertex.GetPointIds().SetId(1, 1)
        aPolyVertex.GetPointIds().SetId(2, 2)

        bPolyVertex = aPolyVertex.NewInstance()
        bPolyVertex.DeepCopy(aPolyVertex)

        aPolyVertexGrid = svtk.svtkUnstructuredGrid()
        aPolyVertexGrid.Allocate(1, 1)
        aPolyVertexGrid.InsertNextCell(aPolyVertex.GetCellType(), aPolyVertex.GetPointIds())
        aPolyVertexGrid.SetPoints(polyVertexPoints)

        aPolyVertexMapper = svtk.svtkDataSetMapper()
        aPolyVertexMapper.SetInputData(aPolyVertexGrid)

        aPolyVertexActor = svtk.svtkActor()
        aPolyVertexActor.SetMapper(aPolyVertexMapper)
        aPolyVertexActor.AddPosition(2, 0, 6)
        aPolyVertexActor.GetProperty().BackfaceCullingOn()

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

        bPenta = aPenta.NewInstance()
        bPenta.DeepCopy(aPenta)

        aPentaGrid = svtk.svtkUnstructuredGrid()
        aPentaGrid.Allocate(1, 1)
        aPentaGrid.InsertNextCell(aPenta.GetCellType(), aPenta.GetPointIds())
        aPentaGrid.SetPoints(pentaPoints)

        aPentaCopy = svtk.svtkUnstructuredGrid()
        aPentaCopy.DeepCopy(aPentaGrid)

        aPentaMapper = svtk.svtkDataSetMapper()
        aPentaMapper.SetInputData(aPentaCopy)

        aPentaActor = svtk.svtkActor()
        aPentaActor.SetMapper(aPentaMapper)
        aPentaActor.AddPosition(10, 0, 0)
        aPentaActor.GetProperty().BackfaceCullingOn()

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

        bHexa = aHexa.NewInstance()
        bHexa.DeepCopy(aHexa)

        aHexaGrid = svtk.svtkUnstructuredGrid()
        aHexaGrid.Allocate(1, 1)
        aHexaGrid.InsertNextCell(aHexa.GetCellType(), aHexa.GetPointIds())
        aHexaGrid.SetPoints(hexaPoints)

        aHexaCopy = svtk.svtkUnstructuredGrid()
        aHexaCopy.DeepCopy(aHexaGrid)

        aHexaMapper = svtk.svtkDataSetMapper()
        aHexaMapper.SetInputData(aHexaCopy)

        aHexaActor = svtk.svtkActor()
        aHexaActor.SetMapper(aHexaMapper)
        aHexaActor.AddPosition(12, 0, 0)
        aHexaActor.GetProperty().BackfaceCullingOn()

        # RIB property
        if hasattr(svtk, 'svtkRIBProperty'):
            aRIBProperty = svtk.svtkRIBProperty()
            aRIBProperty.SetVariable("Km", "float")
            aRIBProperty.SetSurfaceShader("LGVeinedmarble")
            aRIBProperty.SetVariable("veinfreq", "float")
            aRIBProperty.AddVariable("warpfreq", "float")
            aRIBProperty.AddVariable("veincolor", "color")
            aRIBProperty.AddSurfaceShaderParameter("veinfreq", " 2")
            aRIBProperty.AddSurfaceShaderParameter("veincolor", "1.0000 1.0000 0.9412")
            bRIBProperty = svtk.svtkRIBProperty()
            bRIBProperty.SetVariable("Km", "float")
            bRIBProperty.SetSurfaceShaderParameter("Km", "1.0")
            bRIBProperty.SetDisplacementShader("dented")
            bRIBProperty.SetSurfaceShader("plastic")
        aProperty = svtk.svtkProperty()
        bProperty = svtk.svtkProperty()

        aTriangleActor.SetProperty(aProperty)
        aTriangleStripActor.SetProperty(bProperty)

        ren.SetBackground(.1, .2, .4)

        ren.AddActor(aVoxelActor);aVoxelActor.GetProperty().SetDiffuseColor(1, 0, 0)
        ren.AddActor(aHexahedronActor);aHexahedronActor.GetProperty().SetDiffuseColor(1, 1, 0)
        ren.AddActor(aTetraActor);aTetraActor.GetProperty().SetDiffuseColor(0, 1, 0)
        ren.AddActor(aWedgeActor);aWedgeActor.GetProperty().SetDiffuseColor(0, 1, 1)
        ren.AddActor(aPyramidActor);aPyramidActor.GetProperty().SetDiffuseColor(1, 0, 1)
        ren.AddActor(aPixelActor);aPixelActor.GetProperty().SetDiffuseColor(0, 1, 1)
        ren.AddActor(aQuadActor);aQuadActor.GetProperty().SetDiffuseColor(1, 0, 1)
        ren.AddActor(aTriangleActor);aTriangleActor.GetProperty().SetDiffuseColor(.3, 1, .5)
        ren.AddActor(aPolygonActor);aPolygonActor.GetProperty().SetDiffuseColor(1, .4, .5)
        ren.AddActor(aTriangleStripActor);aTriangleStripActor.GetProperty().SetDiffuseColor(.3, .7, 1)
        ren.AddActor(aLineActor);aLineActor.GetProperty().SetDiffuseColor(.2, 1, 1)
        ren.AddActor(aPolyLineActor);aPolyLineActor.GetProperty().SetDiffuseColor(1, 1, 1)
        ren.AddActor(aVertexActor);aVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)
        ren.AddActor(aPolyVertexActor);aPolyVertexActor.GetProperty().SetDiffuseColor(1, 1, 1)
        ren.AddActor(aPentaActor);aPentaActor.GetProperty().SetDiffuseColor(.2, .4, .7)
        ren.AddActor(aHexaActor);aHexaActor.GetProperty().SetDiffuseColor(.7, .5, 1)

        aRIBLight = svtk.svtkRIBLight()
        aRIBLight.SetIntensity(0.7)

        ren.AddLight(aRIBLight)
        aLight = svtk.svtkLight()

        aLight.PositionalOn()
        aLight.SetConeAngle(10.0)
        aLight.SetIntensity(0.7)
        ren.AddLight(aLight)

        ren.ResetCamera()
        ren.GetActiveCamera().Azimuth(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Dolly(2.8)
        ren.ResetCameraClippingRange()

        dir = SVTK_TEMP_DIR

        atext = svtk.svtkTexture()
        pnmReader = svtk.svtkBMPReader()
        pnmReader.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")
        atext.SetInputConnection(pnmReader.GetOutputPort())
        atext.InterpolateOff()
        aTriangleActor.SetTexture(atext)

        aRIBLight.SetFocalPoint(ren.GetActiveCamera().GetFocalPoint())
        aRIBLight.SetPosition(ren.GetActiveCamera().GetPosition())
        aLight.SetFocalPoint(ren.GetActiveCamera().GetFocalPoint())
        aLight.SetPosition(ren.GetActiveCamera().GetPosition())

        # bascially have IO/Export ?
        if hasattr(svtk, 'svtkRIBExporter'):
            rib = svtk.svtkRIBExporter()
            rib.SetInput(renWin)
            rib.SetFilePrefix(dir + '/cells')
            rib.SetTexturePrefix(dir + '/cells')
            rib.Write()

            iv = svtk.svtkIVExporter()
            iv.SetInput(renWin)
            iv.SetFileName(dir + "/cells.iv")
            iv.Write()
            os.remove(dir + '/cells.iv')

            obj = svtk.svtkOBJExporter()
            obj.SetInput(renWin)
            obj.SetFilePrefix(dir + "/cells")
            obj.Write()
            os.remove(dir + '/cells.obj')
            os.remove(dir + '/cells.mtl')

            vrml = svtk.svtkVRMLExporter()
            vrml.SetInput(renWin)
            #vrml.SetStartWrite(vrml.SetFileName(dir + "/cells.wrl"))
            #vrml.SetEndWrite(vrml.SetFileName("/a/acells.wrl"))
            vrml.SetFileName(dir + "/cells.wrl")
            vrml.SetSpeed(5.5)
            vrml.Write()
            os.remove(dir + '/cells.wrl')

            oogl = svtk.svtkOOGLExporter()
            oogl.SetInput(renWin)
            oogl.SetFileName(dir + "/cells.oogl")
            oogl.Write()
            os.remove(dir + '/cells.oogl')

        # the UnRegister calls are because make object is the same as New,
        # and causes memory leaks. (Python does not treat NewInstance the same as New).
        def DeleteCopies():
            bVoxel.UnRegister(None)
            bHexahedron.UnRegister(None)
            bTetra.UnRegister(None)
            bWedge.UnRegister(None)
            bPyramid.UnRegister(None)
            bPixel.UnRegister(None)
            bQuad.UnRegister(None)
            bTriangle.UnRegister(None)
            bPolygon.UnRegister(None)
            bTriangleStrip.UnRegister(None)
            bLine.UnRegister(None)
            bPolyLine.UnRegister(None)
            bVertex.UnRegister(None)
            bPolyVertex.UnRegister(None)
            bPenta.UnRegister(None)
            bHexa.UnRegister(None)


        DeleteCopies()


        # render and interact with data

        renWin.Render()

        img_file = "cells.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file))
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(cells, 'test')])
