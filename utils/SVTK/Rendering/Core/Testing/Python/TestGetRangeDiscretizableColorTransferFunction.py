#!/usr/bin/env python

import svtk
from svtk.test import Testing

class TestGetRangeDiscretizableColorTransferFunction(Testing.svtkTest):
    def testGetRangeDoubleStarArg(self):
        cmap = svtk.svtkDiscretizableColorTransferFunction()

        localRange = [-1, -1]
        cmap.GetRange(localRange)
        self.assertEqual(localRange[0], 0.0)
        self.assertEqual(localRange[1], 0.0)

    def testGetRangeTwoDoubleStarArg(self):
        cmap = svtk.svtkDiscretizableColorTransferFunction()

        localMin = svtk.reference(-1)
        localMax = svtk.reference(-1)
        cmap.GetRange(localMin, localMax)
        self.assertEqual(localMin, 0.0)
        self.assertEqual(localMax, 0.0)

    def testGetRangeNoArg(self):
        cmap = svtk.svtkDiscretizableColorTransferFunction()

        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 0.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeDiscretizableColorTransferFunction, 'test')])
