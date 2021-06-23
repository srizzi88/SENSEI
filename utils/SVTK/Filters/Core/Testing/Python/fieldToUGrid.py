#!/usr/bin/env python
import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Read a field representing unstructured grid and display it (similar to blow.tcl)

# create a reader and write out field data
reader = svtk.svtkUnstructuredGridReader()
reader.SetFileName(SVTK_DATA_ROOT + "/Data/blow.svtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")

ds2do = svtk.svtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())

# we must be able to write here
try:
    channel = open("UGridField.svtk", "wb")
    channel.close()

    write = svtk.svtkDataObjectWriter()
    write.SetInputConnection(ds2do.GetOutputPort())
    write.SetFileName("UGridField.svtk")
    write.Write()

    # Read the field and convert to unstructured grid.
    dor = svtk.svtkDataObjectReader()
    dor.SetFileName("UGridField.svtk")

    do2ds = svtk.svtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToUnstructuredGrid()
    do2ds.SetPointComponent(0, "Points", 0)
    do2ds.SetPointComponent(1, "Points", 1)
    do2ds.SetPointComponent(2, "Points", 2)
    do2ds.SetCellTypeComponent("CellTypes", 0)
    do2ds.SetCellConnectivityComponent("Cells", 0)
    do2ds.Update()

    fd2ad = svtk.svtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputData(do2ds.GetUnstructuredGridOutput())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetVectorComponent(0, "displacement9", 0)
    fd2ad.SetVectorComponent(1, "displacement9", 1)
    fd2ad.SetVectorComponent(2, "displacement9", 2)
    fd2ad.SetScalarComponent(0, "thickness9", 0)
    fd2ad.Update()

    # Now start visualizing
    warp = svtk.svtkWarpVector()
    warp.SetInputData(fd2ad.GetUnstructuredGridOutput())

    # extract mold from mesh using connectivity
    connect = svtk.svtkConnectivityFilter()
    connect.SetInputConnection(warp.GetOutputPort())
    connect.SetExtractionModeToSpecifiedRegions()
    connect.AddSpecifiedRegion(0)
    connect.AddSpecifiedRegion(1)

    moldMapper = svtk.svtkDataSetMapper()
    moldMapper.SetInputConnection(connect.GetOutputPort())
    moldMapper.ScalarVisibilityOff()

    moldActor = svtk.svtkActor()
    moldActor.SetMapper(moldMapper)
    moldActor.GetProperty().SetColor(.2, .2, .2)
    moldActor.GetProperty().SetRepresentationToWireframe()

    # extract parison from mesh using connectivity
    connect2 = svtk.svtkConnectivityFilter()
    connect2.SetInputConnection(warp.GetOutputPort())
    connect2.SetExtractionModeToSpecifiedRegions()
    connect2.AddSpecifiedRegion(2)

    parison = svtk.svtkGeometryFilter()
    parison.SetInputConnection(connect2.GetOutputPort())

    normals2 = svtk.svtkPolyDataNormals()
    normals2.SetInputConnection(parison.GetOutputPort())
    normals2.SetFeatureAngle(60)

    lut = svtk.svtkLookupTable()
    lut.SetHueRange(0.0, 0.66667)

    parisonMapper = svtk.svtkPolyDataMapper()
    parisonMapper.SetInputConnection(normals2.GetOutputPort())
    parisonMapper.SetLookupTable(lut)
    parisonMapper.SetScalarRange(0.12, 1.0)

    parisonActor = svtk.svtkActor()
    parisonActor.SetMapper(parisonMapper)

    cf = svtk.svtkContourFilter()
    cf.SetInputConnection(connect2.GetOutputPort())
    cf.SetValue(0, .5)

    contourMapper = svtk.svtkPolyDataMapper()
    contourMapper.SetInputConnection(cf.GetOutputPort())

    contours = svtk.svtkActor()
    contours.SetMapper(contourMapper)

    # Create graphics stuff
    ren1 = svtk.svtkRenderer()
    renWin = svtk.svtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = svtk.svtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    # Add the actors to the renderer, set the background and size
    ren1.AddActor(moldActor)
    ren1.AddActor(parisonActor)
    ren1.AddActor(contours)

    ren1.ResetCamera()
    ren1.GetActiveCamera().Azimuth(60)
    ren1.GetActiveCamera().Roll(-90)
    ren1.GetActiveCamera().Dolly(2)
    ren1.ResetCameraClippingRange()
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(380, 200)
    renWin.SetMultiSamples(0)
    iren.Initialize()

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("UGridField.svtk")
    except OSError:
        pass

#    iren.Start()

except IOError:
    print("Couldn't open UGridField.svtk for writing.")
