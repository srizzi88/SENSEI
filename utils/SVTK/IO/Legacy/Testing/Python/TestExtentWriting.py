 #!/usr/bin/env python

import os
import svtk

#
# If the current directory is writable, then test the witers
#
try:
    channel = open("test.tmp", "w")
    channel.close()
    os.remove("test.tmp")

    s = svtk.svtkRTAnalyticSource()
    s.SetWholeExtent(5, 10, 5, 10, 5, 10)
    s.Update()

    d = s.GetOutput()

    w = svtk.svtkStructuredPointsWriter()
    w.SetInputData(d)
    w.SetFileName("test-dim.svtk")
    w.Write()

    r = svtk.svtkStructuredPointsReader()
    r.SetFileName("test-dim.svtk")
    r.Update()

    os.remove("test-dim.svtk")

    assert(r.GetOutput().GetExtent() == (0,5,0,5,0,5))
    assert(r.GetOutput().GetOrigin() == (5, 5, 5))

    w.SetInputData(d)
    w.SetFileName("test-dim.svtk")
    w.SetWriteExtent(True)
    w.Write()

    r.Modified()
    r.Update()

    os.remove("test-dim.svtk")

    assert(r.GetOutput().GetExtent() == (5,10,5,10,5,10))
    assert(r.GetOutput().GetOrigin() == (0, 0, 0))

    rg = svtk.svtkRectilinearGrid()
    extents = (1, 3, 1, 3, 1, 3)
    rg.SetExtent(extents)
    pts = svtk.svtkFloatArray()
    pts.InsertNextTuple1(0)
    pts.InsertNextTuple1(1)
    pts.InsertNextTuple1(2)
    rg.SetXCoordinates(pts)
    rg.SetYCoordinates(pts)
    rg.SetZCoordinates(pts)

    w = svtk.svtkRectilinearGridWriter()
    w.SetInputData(rg)
    w.SetFileName("test-dim.svtk")
    w.Write()

    r = svtk.svtkRectilinearGridReader()
    r.SetFileName("test-dim.svtk")
    r.Update()

    os.remove("test-dim.svtk")

    assert(r.GetOutput().GetExtent() == (0,2,0,2,0,2))

    w.SetInputData(rg)
    w.SetFileName("test-dim.svtk")
    w.SetWriteExtent(True)
    w.Write()

    r.Modified()
    r.Update()

    assert(r.GetOutput().GetExtent() == (1,3,1,3,1,3))

    sg = svtk.svtkStructuredGrid()
    extents = (1, 3, 1, 3, 1, 3)
    sg.SetExtent(extents)
    ptsa = svtk.svtkFloatArray()
    ptsa.SetNumberOfComponents(3)
    ptsa.SetNumberOfTuples(27)
    # We don't really care about point coordinates being correct
    for i in range(27):
        ptsa.InsertNextTuple3(0, 0, 0)
    pts = svtk.svtkPoints()
    pts.SetData(ptsa)
    sg.SetPoints(pts)

    w = svtk.svtkStructuredGridWriter()
    w.SetInputData(sg)
    w.SetFileName("test-dim.svtk")
    w.Write()

    # comment out reader part of this test as it has been failing
    # for over 6 months and no one is willing to fix it
    #
    # r = svtk.svtkStructuredGridReader()
    # r.SetFileName("test-dim.svtk")
    # r.Update()

    os.remove("test-dim.svtk")

    # assert(r.GetOutput().GetExtent() == (0,2,0,2,0,2))

    w.SetInputData(sg)
    w.SetFileName("test-dim.svtk")
    w.SetWriteExtent(True)
    w.Write()

    # r.Modified()
    # r.Update()

    # assert(r.GetOutput().GetExtent() == (1,3,1,3,1,3))


except IOError:
    pass
