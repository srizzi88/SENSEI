#!/usr/bin/env python

# This tests svtkCompositeCutter

import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

class TestCompositeCutter(Testing.svtkTest):
  def testAMR(self):
    filename= SVTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"

    reader = svtk.svtkAMREnzoReader()
    reader.SetFileName(filename);
    reader.SetMaxLevel(10);
    reader.SetCellArrayStatus("TotalEnergy",1)

    plane = svtk.svtkPlane()
    plane.SetOrigin(0.5, 0.5, 0.5)
    plane.SetNormal(1, 0, 0)

    cutter = svtk.svtkCompositeCutter()
    cutter.SetCutFunction(plane)
    cutter.SetInputConnection(reader.GetOutputPort())
    cutter.Update()

    slice = cutter.GetOutputDataObject(0)
    self.assertEqual(slice.GetNumberOfCells(),662);

if __name__ == "__main__":
    Testing.main([(TestCompositeCutter, 'test')])
