"""
This test verifies that svtk's Xdmf reader will read a polyhedron sample file.
"""

from __future__ import print_function

import os, sys
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

if __name__ == "__main__":
    fnameIn = "" + str(SVTK_DATA_ROOT) + "/Data/XDMF/polyhedron.xmf"

    xr = svtk.svtkXdmf3Reader()
    xr.CanReadFile(fnameIn)
    xr.SetFileName(fnameIn)
    xr.UpdateInformation()
    xr.Update()
    ds = xr.GetOutputDataObject(0)
    if not ds:
        print("Got zero output from known good file")
        sys.exit(svtk.SVTK_ERROR)

    print(ds.GetCells())
    print(ds.GetPoints())
