#!/usr/bin/env python

import os
import svtk
from svtk.util.misc import svtkGetDataRoot
from svtk.util.misc import svtkGetTempDir

SVTK_DATA_ROOT = svtkGetDataRoot()
SVTK_TEMP_DIR = svtkGetTempDir()

rt = svtk.svtkRTAnalyticSource()
rt.Update()

inp = rt.GetOutput()
inp.GetInformation().Set(svtk.svtkDataObject.DATA_TIME_STEP(), 11)

file_root = SVTK_TEMP_DIR + '/testxmlfield'
file0 = file_root + ".vti"

w = svtk.svtkXMLImageDataWriter()
w.SetInputData(inp)
w.SetFileName(file0)
w.Write()

r = svtk.svtkXMLImageDataReader()
r.SetFileName(file0)
r.UpdateInformation()
assert(r.GetOutputInformation(0).Get(svtk.svtkStreamingDemandDrivenPipeline.TIME_STEPS(), 0) == 11.0)

os.remove(file0)
