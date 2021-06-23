#!/usr/bin/env python
import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# This example demonstrates the reading of a field and conversion to PolyData
# The output should be the same as polyEx.tcl.

# Create a reader and write out the field
reader = svtk.svtkPolyDataReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/polyEx.svtk")

ds2do = svtk.svtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())

# NOTE: This test only works if the current directory is writable
#
try:
    channel = open("PolyField.svtk", "wb")
    channel.close()

    writer = svtk.svtkDataObjectWriter()
    writer.SetInputConnection(ds2do.GetOutputPort())
    writer.SetFileName("PolyField.svtk")
    writer.Write()

    # create pipeline
    #
    dor = svtk.svtkDataObjectReader()
    dor.SetFileName("PolyField.svtk")

    do2ds = svtk.svtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToPolyData()
    do2ds.SetPointComponent(0, "Points", 0)
    do2ds.SetPointComponent(1, "Points", 1)
    do2ds.SetPointComponent(2, "Points", 2)
    do2ds.SetPolysComponent("Polys", 0)

    fd2ad = svtk.svtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputConnection(do2ds.GetOutputPort())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetScalarComponent(0, "my_scalars", 0)

    mapper = svtk.svtkPolyDataMapper()
    mapper.SetInputConnection(fd2ad.GetOutputPort())
    mapper.SetScalarRange(fd2ad.GetOutput().GetScalarRange())

    actor = svtk.svtkActor()
    actor.SetMapper(mapper)

    # Create the RenderWindow, Renderer and both Actors
    ren1 = svtk.svtkRenderer()
    renWin = svtk.svtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = svtk.svtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    ren1.AddActor(actor)
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(300, 300)

    ren1.ResetCamera()
    cam1 = ren1.GetActiveCamera()
    cam1.SetClippingRange(.348, 17.43)
    cam1.SetPosition(2.92, 2.62, -0.836)
    cam1.SetViewUp(-0.436, -0.067, -0.897)
    cam1.Azimuth(90)

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("PolyField.svtk")
    except OSError:
        pass


#    iren.Start()

except IOError:
    print("Couldn't open PolyField.svtk for writing.")
