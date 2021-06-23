import unittest

from svtk.svtkCommonCore import svtkPoints, svtkDoubleArray, svtkIdList
from svtk.svtkCommonDataModel import svtkPlane,\
                                   svtkUnstructuredGrid,\
                                   svtkStructuredGrid,\
                                   svtkPolyData
from svtk.svtkFiltersGeneral import svtkClipDataSet
from svtk.svtkFiltersGeneral import svtkTableBasedClipDataSet
import svtk.util.svtkConstants as svtk_const

class FiltersLosingPrecisionBase:
    def test_clip(self):
        p = svtkPlane()
        p.SetOrigin(0,0,0)
        p.SetNormal(1,0,0)
        f = svtkClipDataSet()
        f.SetInputData(self.cell)
        f.SetClipFunction(p)
        f.SetValue(0)
        f.Update()
        self.assertEquals(f.GetOutput().GetPoints().GetDataType(), svtk_const.SVTK_DOUBLE)

    def test_tablebasedclip(self):
        p = svtkPlane()
        p.SetOrigin(0,0,0)
        p.SetNormal(1,0,0)
        f = svtkTableBasedClipDataSet()
        f.SetInputData(self.cell)
        f.SetClipFunction(p)
        f.SetValue(0)
        f.Update()
        self.assertEquals(f.GetOutput().GetPoints().GetDataType(), svtk_const.SVTK_DOUBLE)

class TestUnstructuredGridFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = svtkUnstructuredGrid()
        pts = svtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0,  1.0,  1.0)
        pts.InsertNextPoint(-1.0,  1.0,  1.0)
        self.cell.SetPoints(pts)
        self.cell.Allocate(1,1)
        ids = svtkIdList()
        for i in range(8):
            ids.InsertId(i,i)
        self.cell.InsertNextCell(svtk_const.SVTK_HEXAHEDRON, ids)
        scalar = svtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 0.0)
        scalar.SetValue(3, 0.0)
        scalar.SetValue(4, 1.0)
        scalar.SetValue(5, 1.0)
        scalar.SetValue(6, 1.0)
        scalar.SetValue(7, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

class TestStructuredGridFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = svtkStructuredGrid()
        pts = svtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0, -1.0,  1.0)
        pts.InsertNextPoint( 1.0,  1.0,  1.0)
        pts.InsertNextPoint(-1.0,  1.0,  1.0)
        self.cell.SetDimensions(2,2,2)
        self.cell.SetPoints(pts)
        scalar = svtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 0.0)
        scalar.SetValue(3, 0.0)
        scalar.SetValue(4, 1.0)
        scalar.SetValue(5, 1.0)
        scalar.SetValue(6, 1.0)
        scalar.SetValue(7, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

class TestPolyDataFiltersLosingPrecision(unittest.TestCase, FiltersLosingPrecisionBase):
    def setUp(self):
        self.cell = svtkPolyData()
        pts = svtkPoints()
        pts.SetDataTypeToDouble()
        pts.InsertNextPoint(-1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0, -1.0, -1.0)
        pts.InsertNextPoint( 1.0,  1.0, -1.0)
        pts.InsertNextPoint(-1.0,  1.0, -1.0)
        self.cell.SetPoints(pts)
        self.cell.Allocate(1,1)
        ids = svtkIdList()
        for i in range(4):
            ids.InsertId(i,i)
        self.cell.InsertNextCell(svtk_const.SVTK_QUAD, ids)
        scalar = svtkDoubleArray()
        scalar.SetName('scalar')
        scalar.SetNumberOfTuples(8)
        scalar.SetValue(0, 0.0)
        scalar.SetValue(1, 0.0)
        scalar.SetValue(2, 1.0)
        scalar.SetValue(3, 1.0)
        self.cell.GetPointData().SetScalars(scalar)

if __name__ == '__main__':
    unittest.main()
