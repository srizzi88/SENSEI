#!/usr/bin/env python

import svtk
import sys

def CheckFilter(inputDS):
    numOrigCells = inputDS.GetNumberOfCells()
    ghostArray = svtk.svtkUnsignedCharArray()
    ghostArray.SetNumberOfTuples(numOrigCells)
    ghostArray.SetName(svtk.svtkDataSetAttributes.GhostArrayName())
    ghostArray.Fill(0)

    inputDS.GetCellData().AddArray(ghostArray)

    removeGhosts = svtk.svtkRemoveGhosts()
    removeGhosts.SetInputDataObject(inputDS)
    removeGhosts.Update()

    outPD = removeGhosts.GetOutput()

    if outPD.GetNumberOfCells() != numOrigCells:
        print("Should have the same amount of cells but did not", outPD.GetNumberOfCells(), numOrigCells)
        sys.exit(1)

    ghostArray.SetValue(0, 1)
    ghostArray.Modified()
    removeGhosts.Modified()
    removeGhosts.Update()

    if outPD.GetNumberOfCells() != numOrigCells-1:
        print("Should have had one less cell but did not", outPD.GetNumberOfCells(), numOrigCells)
        sys.exit(1)


# =================== testing polydata ========================

disk = svtk.svtkDiskSource()
disk.SetRadialResolution(2)
disk.SetCircumferentialResolution(9)

disk.Update()

CheckFilter(disk.GetOutput())

# =================== testing unstructured grid ========================

cellTypeSource = svtk.svtkCellTypeSource()
cellTypeSource.SetBlocksDimensions(4, 5, 6)

cellTypeSource.Update()

CheckFilter(cellTypeSource.GetOutput())

print("SUCCESS")
sys.exit(0)
