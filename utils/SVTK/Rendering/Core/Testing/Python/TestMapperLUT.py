#!/usr/bin/env python

"""This test ensures that the mapper does not change the LookupTable
that may be shared among different mappers.  Specifically, the svtkMapper
and svtkScalarsToColorsPainter classes use the LookupTable to map scalars
to colors.  When they do this they set the LookupTable's Alpha ivar.
They should do it in a manner that the LUT's original value is reset.
If this is not done strange errors show up.  For example if you use an
ImagePlaneWidget and share its LUT with another mapper that has either a
different opacity, then the image plane widget will pick up the wrong
opacity leading to subtle errors.  This test ensures that this does not
happen by testing MapScalars explicitly and also by creating an
ImagePlaneWidget.  """

import os
import os.path
import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestMapperLUT(Testing.svtkTest):
    def testMapScalars(self):
        """Test if the mapper sets the Alpha of the LUT."""
        # Create dummy data.
        p = svtk.svtkPolyData()
        pts = svtk.svtkPoints()
        pts.InsertNextPoint((0,0,0))
        sc = svtk.svtkFloatArray()
        sc.InsertNextValue(10.0)
        p.SetPoints(pts)
        p.GetPointData().SetScalars(sc)
        l = svtk.svtkLookupTable()
        m = svtk.svtkPolyDataMapper()
        m.SetInputData(p)
        m.SetScalarRange(0.0, 10.0)
        m.SetLookupTable(l)
        ret = m.MapScalars(0.5)
        self.assertEqual(l.GetAlpha(), 1.0)

    def testImagePlaneWidget(self):
        "A more rigorous test using the image plane widget."
        # This test is largely copied from
        # Widgets/Python/TestImagePlaneWidget.py

        # Load some data.
        v16 = svtk.svtkVolume16Reader()
        v16.SetDataDimensions(64, 64)
        v16.SetDataByteOrderToLittleEndian()
        v16.SetFilePrefix(os.path.join(SVTK_DATA_ROOT,
                                       "Data", "headsq", "quarter"))
        v16.SetImageRange(1, 93)
        v16.SetDataSpacing(3.2, 3.2, 1.5)
        v16.Update()

        xMin, xMax, yMin, yMax, zMin, zMax = v16.GetExecutive().GetWholeExtent(v16.GetOutputInformation(0))
        img_data = v16.GetOutput()
        spacing = img_data.GetSpacing()
        sx, sy, sz = spacing

        origin = img_data.GetOrigin()
        ox, oy, oz = origin

        # An outline is shown for context.
        outline = svtk.svtkOutlineFilter()
        outline.SetInputData(img_data)

        outlineMapper = svtk.svtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())

        outlineActor = svtk.svtkActor()
        outlineActor.SetMapper(outlineMapper)

        # The shared picker enables us to use 3 planes at one time
        # and gets the picking order right
        picker = svtk.svtkCellPicker()
        picker.SetTolerance(0.005)

        # The 3 image plane widgets are used to probe the dataset.
        planeWidgetX = svtk.svtkImagePlaneWidget()
        planeWidgetX.DisplayTextOn()
        planeWidgetX.SetInputData(img_data)
        planeWidgetX.SetPlaneOrientationToXAxes()
        planeWidgetX.SetSliceIndex(32)
        planeWidgetX.SetPicker(picker)
        planeWidgetX.SetKeyPressActivationValue("x")
        prop1 = planeWidgetX.GetPlaneProperty()
        prop1.SetColor(1, 0, 0)

        planeWidgetY = svtk.svtkImagePlaneWidget()
        planeWidgetY.DisplayTextOn()
        planeWidgetY.SetInputData(img_data)
        planeWidgetY.SetPlaneOrientationToYAxes()
        planeWidgetY.SetSliceIndex(32)
        planeWidgetY.SetPicker(picker)
        planeWidgetY.SetKeyPressActivationValue("y")
        prop2 = planeWidgetY.GetPlaneProperty()
        prop2.SetColor(1, 1, 0)
        planeWidgetY.SetLookupTable(planeWidgetX.GetLookupTable())

        # for the z-slice, turn off texture interpolation:
        # interpolation is now nearest neighbour, to demonstrate
        # cross-hair cursor snapping to pixel centers
        planeWidgetZ = svtk.svtkImagePlaneWidget()
        planeWidgetZ.DisplayTextOn()
        planeWidgetZ.SetInputData(img_data)
        planeWidgetZ.SetPlaneOrientationToZAxes()
        planeWidgetZ.SetSliceIndex(46)
        planeWidgetZ.SetPicker(picker)
        planeWidgetZ.SetKeyPressActivationValue("z")
        prop3 = planeWidgetZ.GetPlaneProperty()
        prop3.SetColor(0, 0, 1)
        planeWidgetZ.SetLookupTable(planeWidgetX.GetLookupTable())

        # Now create another actor with an opacity < 1 and with some
        # scalars.
        p = svtk.svtkPolyData()
        pts = svtk.svtkPoints()
        pts.InsertNextPoint((0,0,0))
        sc = svtk.svtkFloatArray()
        sc.InsertNextValue(1.0)
        p.SetPoints(pts)
        p.GetPointData().SetScalars(sc)
        m = svtk.svtkPolyDataMapper()
        m.SetInputData(p)
        # Share the lookup table of the widgets.
        m.SetLookupTable(planeWidgetX.GetLookupTable())
        m.UseLookupTableScalarRangeOn()
        dummyActor = svtk.svtkActor()
        dummyActor.SetMapper(m)
        dummyActor.GetProperty().SetOpacity(0.0)

        # Create the RenderWindow and Renderer
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        # Add the dummy actor.
        ren.AddActor(dummyActor)
        # Add the outline actor to the renderer, set the background
        # color and size
        ren.AddActor(outlineActor)
        renWin.SetSize(600, 600)
        ren.SetBackground(0.1, 0.1, 0.2)

        current_widget = planeWidgetZ
        mode_widget = planeWidgetZ

        # Set the interactor for the widgets
        iact = svtk.svtkRenderWindowInteractor()
        iact.SetRenderWindow(renWin)
        planeWidgetX.SetInteractor(iact)
        planeWidgetX.On()
        planeWidgetY.SetInteractor(iact)
        planeWidgetY.On()
        planeWidgetZ.SetInteractor(iact)
        planeWidgetZ.On()

        # Create an initial interesting view
        ren.ResetCamera();
        cam1 = ren.GetActiveCamera()
        cam1.Elevation(110)
        cam1.SetViewUp(0, 0, -1)
        cam1.Azimuth(45)
        ren.ResetCameraClippingRange()

        iact.Initialize()
        renWin.Render()

if __name__ == "__main__":
    Testing.main([(TestMapperLUT, 'test')])

