#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()


def addGlobalIds(dataset):
    ids = svtk.svtkIdFilter()
    ids.PointIdsOn()
    ids.CellIdsOn()
    ids.SetPointIdsArrayName("svtkGlobalIds")
    ids.SetCellIdsArrayName("svtkGlobalIds")
    ids.SetInputDataObject(dataset)
    ids.Update()

    data2 = ids.GetOutput()
    dataset.GetCellData().SetGlobalIds(data2.GetCellData().GetArray("svtkGlobalIds"))
    dataset.GetPointData().SetGlobalIds(data2.GetPointData().GetArray("svtkGlobalIds"))

reader = svtk.svtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,64)
reader.SetFilePrefix("" + str(SVTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1.6,1.6,1.5)
clipper = svtk.svtkImageClip()
clipper.SetInputConnection(reader.GetOutputPort())
clipper.SetOutputWholeExtent(30,36,30,36,30,36)

tris = svtk.svtkDataSetTriangleFilter()
tris.SetInputConnection(clipper.GetOutputPort())
tris.UpdatePiece(0, 8, 1);

ugdata = tris.GetOutput()
addGlobalIds(ugdata)

# confirm global ids are preserved for unstructured grid
assert ugdata.IsA("svtkUnstructuredGrid")
assert ugdata.GetCellData().GetGlobalIds() != None and ugdata.GetPointData().GetGlobalIds() != None
ugdata.RemoveGhostCells()
assert ugdata.GetCellData().GetGlobalIds() != None and ugdata.GetPointData().GetGlobalIds() != None

# confirm global ids are preserved for poly data
geom = svtk.svtkGeometryFilter()
geom.SetInputConnection(tris.GetOutputPort())
geom.UpdatePiece(0, 8, 1);

pddata = geom.GetOutput()
addGlobalIds(pddata)

# confirm global ids are preserved for unstructured grid
assert pddata.IsA("svtkPolyData")
assert pddata.GetCellData().GetGlobalIds() != None and pddata.GetPointData().GetGlobalIds() != None
pddata.RemoveGhostCells()
assert pddata.GetCellData().GetGlobalIds() != None and pddata.GetPointData().GetGlobalIds() != None
