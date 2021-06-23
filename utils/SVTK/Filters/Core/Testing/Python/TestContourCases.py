# This test requires Numpy.

import sys
import svtk
from svtk.test import Testing
try:
    import numpy as np
except ImportError:
    print("WARNING: This test requires Numeric Python: http://numpy.sf.net")
    from svtk.test import Testing
    Testing.skip()

def GenerateCell(cellType, points):
    cell = svtk.svtkUnstructuredGrid()
    pts = svtk.svtkPoints()
    for p in points:
        pts.InsertNextPoint(p)
    cell.SetPoints(pts)
    cell.Allocate(1,1)
    ids = svtk.svtkIdList()
    for i in range(len(points)):
        ids.InsertId(i,i)
    cell.InsertNextCell(cellType, ids)
    return cell

def Combination(sz, n):
   c = np.zeros(sz)
   i = 0
   while n>0:
     c[i] = n%2
     n=n>>1
     i = i + 1
   return c

class CellTestBase:
    def test_contours(self):
        cell = svtk.svtkUnstructuredGrid()
        cell.ShallowCopy(self.Cell)

        np = self.Cell.GetNumberOfPoints()
        ncomb = pow(2, np)

        scalar = svtk.svtkDoubleArray()
        scalar.SetName("scalar")
        scalar.SetNumberOfTuples(np)
        cell.GetPointData().SetScalars(scalar)

        incorrectCases = []
        for i in range(1,ncomb-1):
            c = Combination(np, i)
            for p in range(np):
                scalar.SetTuple1(p, c[p])

            gradientFilter = svtk.svtkGradientFilter()
            gradientFilter.SetInputData(cell)
            gradientFilter.SetInputArrayToProcess(0,0,0,0,'scalar')
            gradientFilter.SetResultArrayName('grad')
            gradientFilter.Update()

            contourFilter = svtk.svtkContourFilter()
            contourFilter.SetInputConnection(gradientFilter.GetOutputPort())
            contourFilter.SetNumberOfContours(1)
            contourFilter.SetValue(0, 0.5)
            contourFilter.Update()

            normalsFilter = svtk.svtkPolyDataNormals()
            normalsFilter.SetInputConnection(contourFilter.GetOutputPort())
            normalsFilter.SetConsistency(0)
            normalsFilter.SetFlipNormals(0)
            normalsFilter.SetSplitting(0)

            calcFilter = svtk.svtkArrayCalculator()
            calcFilter.SetInputConnection(normalsFilter.GetOutputPort())
            calcFilter.SetAttributeTypeToPointData()
            calcFilter.AddVectorArrayName('grad')
            calcFilter.AddVectorArrayName('Normals')
            calcFilter.SetResultArrayName('dir')
            calcFilter.SetFunction('grad.Normals')
            calcFilter.Update()

            out = svtk.svtkUnstructuredGrid()
            out.ShallowCopy(calcFilter.GetOutput())

            numPts = out.GetNumberOfPoints()
            if numPts > 0:
                dirArray = out.GetPointData().GetArray('dir')
                for p in range(numPts):
                    if(dirArray.GetTuple1(p) > 0.0): # all normals are reversed
                        incorrectCases.append(i)
                        break

        self.assertEquals(','.join([str(i) for i in incorrectCases]), '')

class TestTetra(Testing.svtkTest, CellTestBase):
    def setUp(self):
        self.Cell = GenerateCell(svtk.SVTK_TETRA,
            [ ( 1.0, -1.0, -1.0),
              ( 1.0,  1.0,  1.0),
              (-1.0,  1.0, -1.0),
              (-1.0, -1.0,  1.0) ])

class TestHexahedron(Testing.svtkTest, CellTestBase):
    def setUp(self):
        self.Cell = GenerateCell(svtk.SVTK_HEXAHEDRON,
            [ (-1.0, -1.0, -1.0),
              ( 1.0, -1.0, -1.0),
              ( 1.0,  1.0, -1.0),
              (-1.0,  1.0, -1.0),
              (-1.0, -1.0,  1.0),
              ( 1.0, -1.0,  1.0),
              ( 1.0,  1.0,  1.0),
              (-1.0,  1.0,  1.0) ])

class TestWedge(Testing.svtkTest, CellTestBase):
    def setUp(self):
        self.Cell = GenerateCell(svtk.SVTK_WEDGE,
            [ (-1.0, -1.0, -1.0),
              ( 1.0, -1.0, -1.0),
              ( 0.0, -1.0,  1.0),
              (-1.0,  1.0, -1.0),
              ( 1.0,  1.0, -1.0),
              ( 0.0,  1.0,  0.0) ])

class TestPyramid(Testing.svtkTest, CellTestBase):
    def setUp(self):
        self.Cell = GenerateCell(svtk.SVTK_PYRAMID,
            [ (-1.0, -1.0, -1.0),
              ( 1.0, -1.0, -1.0),
              ( 1.0,  1.0, -1.0),
              (-1.0,  1.0, -1.0),
              ( 0.0,  0.0,  1.0) ])

if __name__ == '__main__':
    Testing.main([(TestPyramid,'test'),(TestWedge,'test'),(TestTetra, 'test'),(TestHexahedron,'test')])
