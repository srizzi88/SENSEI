#!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.misc import svtkGetTempDir

SVTK_DATA_ROOT = svtkGetDataRoot()
SVTK_TEMP_DIR = svtkGetTempDir()

w = svtk.svtkRTAnalyticSource()

g = svtk.svtkMultiBlockDataGroupFilter()
g.AddInputConnection(w.GetOutputPort())

g.Update()

mb = g.GetOutputDataObject(0)

a = svtk.svtkFloatArray()
a.SetName("foo")
a.SetNumberOfTuples(1)
a.SetValue(0, 10)

mb.GetFieldData().AddArray(a)

file_root = SVTK_TEMP_DIR + '/testxml'
file0 = file_root + ".vtm"

wri = svtk.svtkXMLMultiBlockDataWriter()
wri.SetInputData(mb)
wri.SetFileName(file0)
wri.Write()

read = svtk.svtkXMLMultiBlockDataReader()
read.SetFileName(file0)
read.Update()

output = read.GetOutputDataObject(0)
assert(output.GetFieldData().GetArray("foo"))
assert(output.GetFieldData().GetArray("foo").GetValue(0) == 10)

os.remove(file0)
os.remove(file_root + "/testxml_0.vti")
os.rmdir(file_root)
