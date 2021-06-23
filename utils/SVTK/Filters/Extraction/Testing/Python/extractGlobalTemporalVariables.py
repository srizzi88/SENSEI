#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtk.svtkExodusIIReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/can.ex2")
reader.UpdateInformation()

for i in range(reader.GetNumberOfObjectArrays(reader.GLOBAL)):
    name = reader.GetObjectArrayName(reader.GLOBAL, i)
    reader.SetObjectArrayStatus(reader.GLOBAL, name, 1)

extractTFD = svtk.svtkExtractExodusGlobalTemporalVariables()
extractTFD.SetInputConnection(reader.GetOutputPort())
extractTFD.Update()

data = extractTFD.GetOutputDataObject(0)
assert data.IsA("svtkTable")
assert data.GetNumberOfRows() == reader.GetNumberOfTimeSteps()
assert data.GetNumberOfColumns() > 0
