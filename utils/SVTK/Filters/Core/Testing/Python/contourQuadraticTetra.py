

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import svtk
from svtk.test import Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Contour the quadratic tet type
class contourQuadraticTetra(svtk.test.Testing.svtkTest):

    def testContourQuadraticTetra(self):
        # Create a reader to load the data (quadratic tetrahedra)
        reader = svtk.svtkUnstructuredGridReader()
        reader.SetFileName(SVTK_DATA_ROOT + "/Data/quadTetEdgeTest.svtk")

        tetContours = svtk.svtkContourFilter()
        tetContours.SetInputConnection(reader.GetOutputPort())
        tetContours.SetValue(0, 0.5)
        aTetContourMapper = svtk.svtkDataSetMapper()
        aTetContourMapper.SetInputConnection(tetContours.GetOutputPort())
        aTetContourMapper.ScalarVisibilityOff()
        aTetMapper = svtk.svtkDataSetMapper()
        aTetMapper.SetInputConnection(reader.GetOutputPort())
        aTetMapper.ScalarVisibilityOff()
        aTetActor = svtk.svtkActor()
        aTetActor.SetMapper(aTetMapper)
        aTetActor.GetProperty().SetRepresentationToWireframe()
        aTetActor.GetProperty().SetAmbient(1.0)
        aTetContourActor = svtk.svtkActor()
        aTetContourActor.SetMapper(aTetContourMapper)
        aTetContourActor.GetProperty().SetAmbient(1.0)

        # Create the rendering related stuff.
        # Since some of our actors are a single vertex, we need to remove all
        # cullers so the single vertex actors will render
        ren1 = svtk.svtkRenderer()
        ren1.GetCullers().RemoveAllItems()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren1)
        iren = svtk.svtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        ren1.SetBackground(.1, .2, .3)

        renWin.SetSize(400, 250)

        # specify properties
        ren1.AddActor(aTetActor)
        ren1.AddActor(aTetContourActor)

        ren1.ResetCamera()
        ren1.GetActiveCamera().Dolly(1.5)
        ren1.ResetCameraClippingRange()

        renWin.Render()

        img_file = "contourQuadraticTetra.png"
        svtk.test.Testing.compareImage(iren.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(contourQuadraticTetra, 'test')])
