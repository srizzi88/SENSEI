#!/usr/bin/env python
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Explicitly test contouring of svtkPolyData
class TestContourPolyData(Testing.svtkTest):
  def testAll(self):
    reader = svtk.svtkXMLPolyDataReader()
    reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/disk_out_ref_surface.vtp")
    reader.SetPointArrayStatus("Temp", 1);
    reader.SetPointArrayStatus("Pres", 1);
    reader.Update();

    input = reader.GetOutput()

    cf = svtk.svtkContourFilter()
    cf.SetInputData(input)
    cf.SetValue(0,400)
    cf.SetInputArrayToProcess(0, 0, 0, 0, "Temp");
    cf.GenerateTrianglesOff()
    cf.Update()

    self.assertEqual(cf.GetOutput().GetNumberOfPoints(), 36)
    self.assertEqual(cf.GetOutput().GetNumberOfCells(), 36)

    cf.GenerateTrianglesOn()
    cf.Update()

    self.assertEqual(cf.GetOutput().GetNumberOfPoints(), 36)
    self.assertEqual(cf.GetOutput().GetNumberOfCells(), 36)

    # Check that expected arrays are in the output

    # The active scalars in the input shouldn't effect the
    # scalars in the output
    availableScalars = ["Temp", "Pres"]
    for scalar in availableScalars:

      input.GetPointData().SetActiveScalars(scalar)

      cf.ComputeScalarsOn()
      cf.Update()

      pd = cf.GetOutput().GetPointData()
      self.assertNotEqual(pd.GetArray("Temp"), None)
      self.assertNotEqual(pd.GetArray("Pres"), None)

      cf.ComputeScalarsOff() # "Temp" array should not be in output
      cf.Update()

      pd = cf.GetOutput().GetPointData()
      self.assertEqual(pd.GetArray("Temp"), None)
      self.assertNotEqual(pd.GetArray("Pres"), None)

if __name__ == "__main__":
  Testing.main([(TestContourPolyData, 'test')])
