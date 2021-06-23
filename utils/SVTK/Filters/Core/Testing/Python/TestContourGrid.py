#!/usr/bin/env python
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestContourGrid(Testing.svtkTest):
  def testAll(self):
    reader = svtk.svtkExodusIIReader()
    reader.SetFileName("" + str(SVTK_DATA_ROOT) + "/Data/disk_out_ref.ex2")
    reader.SetPointResultArrayStatus("Temp", 1);
    reader.SetPointResultArrayStatus("Pres", 1);
    reader.Update();

    input = reader.GetOutput().GetBlock(0).GetBlock(0);

    cf = svtk.svtkContourFilter()
    cf.SetInputData(input)
    cf.SetValue(0,400)
    cf.SetInputArrayToProcess(0, 0, 0, 0, "Temp");
    cf.GenerateTrianglesOff()
    cf.Update()
    self.assertEqual(cf.GetOutput().GetNumberOfPoints(), 1047)
    self.assertEqual(cf.GetOutput().GetNumberOfCells(), 1028)

    cf.GenerateTrianglesOn()
    cf.Update()
    self.assertEqual(cf.GetOutput().GetNumberOfPoints(), 1047)
    self.assertEqual(cf.GetOutput().GetNumberOfCells(), 2056)

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
  Testing.main([(TestContourGrid, 'test')])
