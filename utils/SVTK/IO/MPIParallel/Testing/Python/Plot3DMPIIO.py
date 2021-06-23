#!/usr/bin/env python
import sys
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

def Gather(c, arr, root):
    svtkArr = svtk.svtkDoubleArray()
    count = len(arr)
    svtkArr.SetNumberOfTuples(count)
    for i in range(count):
        svtkArr.SetValue(i, arr[i])
    svtkResult = svtk.svtkDoubleArray()
    c.Gather(svtkArr, svtkResult, root)
    result = [svtkResult.GetValue(i) for i in range(svtkResult.GetNumberOfTuples())]
    return [ tuple(result[i : i + count]) \
                for i in range(0, svtkResult.GetNumberOfTuples(), count) ]

renWin = svtk.svtkRenderWindow()

iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

r = svtk.svtkMultiBlockPLOT3DReader()
# Since svtkMPIMultiBlockPLOT3DReader is not created on Windows even when MPI
# is enabled.
assert r.IsA("svtkMPIMultiBlockPLOT3DReader") == 1 or sys.platform == "win32"

r.SetFileName(SVTK_DATA_ROOT + "/Data/multi-bin.xyz")
r.SetQFileName(SVTK_DATA_ROOT + "/Data/multi-bin-oflow.q")
r.SetFunctionFileName(SVTK_DATA_ROOT + "/Data/multi-bin.f")
r.AutoDetectFormatOn()

r.Update()

c = svtk.svtkMPIController.GetGlobalController()
size = c.GetNumberOfProcesses()
rank = c.GetLocalProcessId()
block = 0

bounds = r.GetOutput().GetBlock(block).GetBounds()
bounds = Gather(c, bounds, root=0)

if rank == 0:
    print("Reader:", r.GetClassName())
    print("Bounds:")
    for i in range(size):
        print(bounds[i])

c.Barrier()
aname = "StagnationEnergy"
rng = r.GetOutput().GetBlock(block).GetPointData().GetArray(aname).GetRange(0)

rng = Gather(c, rng, root=0)
if rank == 0:
    print("StagnationEnergy Ranges:")
    for i in range(size):
        print(rng[i])
        assert rng[i][0] > 1.1 and rng[i][0] < 24.1 and \
               rng[i][1] > 1.1 and rng[i][1] < 24.1
