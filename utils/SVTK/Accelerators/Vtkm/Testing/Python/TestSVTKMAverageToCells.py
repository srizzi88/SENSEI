import sys

try:
    import numpy
except ImportError:
    print("Numpy (http://numpy.scipy.org) not found.")
    print("This test requires numpy!")
    from svtk.test import Testing
    Testing.skip()

import svtk
from svtk.numpy_interface import dataset_adapter as dsa
from svtk.numpy_interface import algorithms as algs

def test_dataset(ds):
  p2c = svtk.svtkPointDataToCellData()
  p2c.SetInputData(ds)
  p2c.Update()

  d1 = dsa.WrapDataObject(p2c.GetOutput())

  svtkm_p2c = svtk.svtkmAverageToCells()
  svtkm_p2c.SetInputData(ds)
  svtkm_p2c.SetInputArrayToProcess(0, 0, 0, svtk.svtkDataObject.FIELD_ASSOCIATION_POINTS, "RTData")
  svtkm_p2c.Update()

  d2 = dsa.WrapDataObject(svtkm_p2c.GetOutput())

  rtD1 = d1.PointData['RTData']
  rtD2 = d2.PointData['RTData']

  assert (algs.max(algs.abs(rtD1 - rtD2)) < 10E-4)

print("Testing simple debugging grid...")
# This dataset matches the example values in svtkmCellSetExplicit:
dbg = svtk.svtkUnstructuredGrid()
dbg.SetPoints(svtk.svtkPoints())
dbg.GetPoints().SetNumberOfPoints(7)
dbg.InsertNextCell(svtk.SVTK_TRIANGLE, 3, [0, 1, 2])
dbg.InsertNextCell(svtk.SVTK_QUAD,     4, [0, 1, 3, 4])
dbg.InsertNextCell(svtk.SVTK_TRIANGLE, 3, [1, 3, 5])
dbg.InsertNextCell(svtk.SVTK_LINE,     2, [5, 6])

dbgRt = svtk.svtkDoubleArray()
dbgRt.SetNumberOfTuples(7)
dbgRt.SetName('RTData')
dbgRt.SetValue(0, 17.40)
dbgRt.SetValue(1, 123.0)
dbgRt.SetValue(2, 28.60)
dbgRt.SetValue(3, 19.47)
dbgRt.SetValue(4, 3.350)
dbgRt.SetValue(5, 0.212)
dbgRt.SetValue(6, 1023.)
dbg.GetPointData().AddArray(dbgRt)

test_dataset(dbg)
print("Success!")

print("Testing homogeneous image data...")
source = svtk.svtkRTAnalyticSource()
source.Update()
imgData = source.GetOutput()
test_dataset(imgData)
print("Success!")

d = dsa.WrapDataObject(imgData)
rtData = d.PointData['RTData']
rtMin = algs.min(rtData)
rtMax = algs.max(rtData)
clipScalar = 0.5 * (rtMin + rtMax)

print("Testing non-homogeneous unstructured grid...")
clip = svtk.svtkClipDataSet()
clip.SetInputData(imgData)
clip.SetValue(clipScalar)
clip.Update()
ugrid = clip.GetOutput()
test_dataset(ugrid)
print("Success!")
