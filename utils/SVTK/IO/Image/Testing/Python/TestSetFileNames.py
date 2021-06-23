#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

globFileNames = svtk.svtkGlobFileNames()
globFileNames.AddFileNames(SVTK_DATA_ROOT + "/Data/headsq/quarter.*[0-9]")

sortFileNames = svtk.svtkSortFileNames()
sortFileNames.SetInputFileNames(globFileNames.GetFileNames())
sortFileNames.NumericSortOn()

reader = svtk.svtkImageReader2()
reader.SetFileNames(sortFileNames.GetFileNames())
reader.SetDataExtent(0, 63, 0, 63, 1, 1)
reader.SetDataByteOrderToLittleEndian()

# set Z slice to 2: if output is not numerically sorted, the wrong
# slice will be shown
viewer = svtk.svtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetZSlice(2)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.GetRenderer().SetBackground(0, 0, 0)
viewer.Render()
