#!/usr/bin/env python

# Test reading of XML file with timesteps

import svtk.svtkIOXML
from svtk.util.misc import svtkGetDataRoot

SVTK_DATA_ROOT = svtkGetDataRoot()

reader = svtk.svtkIOXML.svtkXMLUnstructuredGridReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/cube-with-time.vtu")
reader.Update()

# Ensure the reader identifies the timesteps in the file
assert reader.GetNumberOfTimeSteps() == 8
