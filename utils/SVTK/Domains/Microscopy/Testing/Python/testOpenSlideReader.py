#!/usr/bin/env python
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()
import sys

class TestOpenSlideReader(svtk.test.Testing.svtkTest):

    def testCanReadFile(self):
        reader = svtk.svtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(SVTK_DATA_ROOT + "/Data/RectGrid2.svtk"), 0)

    def testCanNotReadFile(self):
        reader = svtk.svtkOpenSlideReader()
        self.assertEqual(reader.CanReadFile(SVTK_DATA_ROOT + "/Data/Microscopy/small2.ndpi"), 2)

if __name__ == "__main__":
    svtk.test.Testing.main([(TestOpenSlideReader, 'test')])
