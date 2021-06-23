#!/usr/bin/env python

# This tests svtkAMRExtractLevel

import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

class TestAMRExtractLevel(Testing.svtkTest):
  def testAMR(self):
    filename= SVTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"
    level = 1

    reader = svtk.svtkAMREnzoReader()
    reader.SetFileName(filename)
    reader.SetMaxLevel(10)
    reader.SetCellArrayStatus("TotalEnergy",1)

    filter = svtk.svtkExtractLevel()
    filter.AddLevel(level)

    filter.SetInputConnection(reader.GetOutputPort())
    filter.Update()

    amr  = reader.GetOutputDataObject(0)
    out = filter.GetOutputDataObject(0)
    self.assertEqual(out.GetNumberOfBlocks(), amr.GetNumberOfDataSets(level))

if __name__ == "__main__":
    Testing.main([(TestAMRExtractLevel, 'test')])
