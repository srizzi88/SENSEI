#!/usr/bin/env python
import os
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# The current directory must be writeable.
#
try:
    filename = "mni-thinplatespline.xfm"
    channel = open(filename, "wb")
    channel.close()

    # first, create an image to warp
    imageGrid = svtk.svtkImageGridSource()
    imageGrid.SetGridSpacing(16, 16, 0)
    imageGrid.SetGridOrigin(0, 0, 0)
    imageGrid.SetDataExtent(0, 255, 0, 255, 0, 0)
    imageGrid.SetDataScalarTypeToUnsignedChar()

    table = svtk.svtkLookupTable()
    table.SetTableRange(0, 1)
    table.SetValueRange(1.0, 0.0)
    table.SetSaturationRange(0.0, 0.0)
    table.SetHueRange(0.0, 0.0)
    table.SetAlphaRange(0.0, 1.0)
    table.Build()

    alpha = svtk.svtkImageMapToColors()
    alpha.SetInputConnection(imageGrid.GetOutputPort())
    alpha.SetLookupTable(table)

    reader1 = svtk.svtkBMPReader()
    reader1.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")

    blend = svtk.svtkImageBlend()
    blend.AddInputConnection(reader1.GetOutputPort())
    blend.AddInputConnection(alpha.GetOutputPort())

    # next, create a ThinPlateSpline transform
    p1 = svtk.svtkPoints()
    p1.SetNumberOfPoints(8)
    p1.SetPoint(0, 0, 0, 0)
    p1.SetPoint(1, 0, 255, 0)
    p1.SetPoint(2, 255, 0, 0)
    p1.SetPoint(3, 255, 255, 0)
    p1.SetPoint(4, 96, 96, 0)
    p1.SetPoint(5, 96, 159, 0)
    p1.SetPoint(6, 159, 159, 0)
    p1.SetPoint(7, 159, 96, 0)

    p2 = svtk.svtkPoints()
    p2.SetNumberOfPoints(8)
    p2.SetPoint(0, 0, 0, 0)
    p2.SetPoint(1, 0, 255, 0)
    p2.SetPoint(2, 255, 0, 0)
    p2.SetPoint(3, 255, 255, 0)
    p2.SetPoint(4, 96, 159, 0)
    p2.SetPoint(5, 159, 159, 0)
    p2.SetPoint(6, 159, 96, 0)
    p2.SetPoint(7, 96, 96, 0)

    thinPlate0 = svtk.svtkThinPlateSplineTransform()
    thinPlate0.SetSourceLandmarks(p1)
    thinPlate0.SetTargetLandmarks(p2)
    thinPlate0.SetBasisToR2LogR()

    # write the tps to a file
    tpsWriter = svtk.svtkMNITransformWriter()
    tpsWriter.SetFileName(filename)
    tpsWriter.SetTransform(thinPlate0)
    tpsWriter.Write()
    # read it back
    tpsReader = svtk.svtkMNITransformReader()
    if (tpsReader.CanReadFile(filename) != 0):
        tpsReader.SetFileName(filename)

        thinPlate = tpsReader.GetTransform()

        # make a linear transform
        linearTransform = svtk.svtkTransform()
        linearTransform.PostMultiply()
        linearTransform.Translate(-127.5, -127.5, 0)
        linearTransform.RotateZ(30)
        linearTransform.Translate(+127.5, +127.5, 0)

        # remove the linear part of the thin plate
        tpsGeneral = svtk.svtkGeneralTransform()
        tpsGeneral.SetInput(thinPlate)
        tpsGeneral.PreMultiply()
        tpsGeneral.Concatenate(linearTransform.GetInverse().GetMatrix())

        # convert the thin plate spline into a grid
        transformToGrid = svtk.svtkTransformToGrid()
        transformToGrid.SetInput(tpsGeneral)
        transformToGrid.SetGridSpacing(16, 16, 1)
        transformToGrid.SetGridOrigin(-64.5, -64.5, 0)
        transformToGrid.SetGridExtent(0, 24, 0, 24, 0, 0)
        transformToGrid.Update()

        gridTransform = svtk.svtkGridTransform()
        gridTransform.SetDisplacementGridConnection(
          transformToGrid.GetOutputPort())
        gridTransform.SetInterpolationModeToCubic()

        # add back the linear part
        gridGeneral = svtk.svtkGeneralTransform()
        gridGeneral.SetInput(gridTransform)
        gridGeneral.PreMultiply()
        gridGeneral.Concatenate(linearTransform.GetMatrix())

        # invert for reslice
        gridGeneral.Inverse()
        # write to a file
        gridWriter = svtk.svtkMNITransformWriter()
        gridWriter.SetFileName("mni-grid.xfm")
        gridWriter.SetComments("TestMNITransforms output transform")
        gridWriter.SetTransform(gridGeneral)
        gridWriter.Write()

        # read it back
        gridReader = svtk.svtkMNITransformReader()
        gridReader.SetFileName("mni-grid.xfm")
        transform = gridReader.GetTransform()

        # apply the grid warp to the image
        reslice = svtk.svtkImageReslice()
        reslice.SetInputConnection(blend.GetOutputPort())
        reslice.SetResliceTransform(transform)
        reslice.SetInterpolationModeToLinear()

        # set the window/level to 255.0/127.5 to view full range
        viewer = svtk.svtkImageViewer()
        viewer.SetInputConnection(reslice.GetOutputPort())
        viewer.SetColorWindow(255.0)
        viewer.SetColorLevel(127.5)
        viewer.SetZSlice(0)
        viewer.Render()

        try:
            os.remove("mni-grid.xfm")
        except OSError:
            pass
        try:
            os.remove("mni-grid_grid.mnc")
        except OSError:
            pass

    try:
        os.remove(filename)
    except OSError:
        pass

except IOError:
    print("Unable to test the writer/reader.")
