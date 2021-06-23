#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastIndependentComponentMIP.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import sys
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the svtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

class TestGPURayCastIndependentComponentMIP(svtk.test.Testing.svtkTest):

    def test(self):
        dataRoot = svtkGetDataRoot()
        reader =  svtk.svtkXMLImageDataReader()
        reader.SetFileName("" + str(dataRoot) + "/Data/vase_4comp.vti")

        volume = svtk.svtkVolume()
        #mapper = svtk.svtkFixedPointVolumeRayCastMapper()
        mapper = svtk.svtkGPUVolumeRayCastMapper()
        mapper.SetBlendModeToMaximumIntensity();
        mapper.SetSampleDistance(0.1)
        mapper.SetAutoAdjustSampleDistances(0)
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        iRen = svtk.svtkRenderWindowInteractor()

        # Set connections
        mapper.SetInputConnection(reader.GetOutputPort());
        volume.SetMapper(mapper)
        ren.AddViewProp(volume)
        renWin.AddRenderer(ren)
        iRen.SetRenderWindow(renWin)

        # Define opacity transfer function and color functions
        opacityFunc1 = svtk.svtkPiecewiseFunction()
        opacityFunc1.AddPoint(0.0, 0.0);
        opacityFunc1.AddPoint(60.0, 0.1);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc2 = svtk.svtkPiecewiseFunction()
        opacityFunc2.AddPoint(0.0, 0.0);
        opacityFunc2.AddPoint(60.0, 0.0);
        opacityFunc2.AddPoint(120.0, 0.1);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc3 = svtk.svtkPiecewiseFunction()
        opacityFunc3.AddPoint(0.0, 0.0);
        opacityFunc3.AddPoint(120.0, 0.0);
        opacityFunc3.AddPoint(180.0, 0.1);
        opacityFunc3.AddPoint(255.0, 0.0);

        opacityFunc4 = svtk.svtkPiecewiseFunction()
        opacityFunc4.AddPoint(0.0, 0.0);
        opacityFunc4.AddPoint(180.0, 0.0);
        opacityFunc4.AddPoint(255.0, 0.1);

        # Color transfer functions
        color1 = svtk.svtkColorTransferFunction()
        color1.AddRGBPoint(0.0, 1.0, 0.0, 0.0);
        color1.AddRGBPoint(60.0, 1.0, 0.0, 0.0);

        color2 = svtk.svtkColorTransferFunction()
        color2.AddRGBPoint(60.0, 0.0, 0.0, 1.0);
        color2.AddRGBPoint(120.0, 0.0, 0.0, 1.0);

        color3 = svtk.svtkColorTransferFunction()
        color3.AddRGBPoint(120.0, 0.0, 1.0, 0.0);
        color3.AddRGBPoint(180.0, 0.0, 1.0, 0.0);

        color4 = svtk.svtkColorTransferFunction()
        color4.AddRGBPoint(180.0, 0.0, 0.0, 0.0);
        color4.AddRGBPoint(239.0, 0.0, 0.0, 0.0);

        # Now set the opacity and the color
        volumeProperty = volume.GetProperty()
        volumeProperty.SetIndependentComponents(1)
        volumeProperty.SetScalarOpacity(0, opacityFunc1)
        volumeProperty.SetScalarOpacity(1, opacityFunc2)
        volumeProperty.SetScalarOpacity(2, opacityFunc3)
        volumeProperty.SetScalarOpacity(3, opacityFunc4)
        volumeProperty.SetColor(0, color1)
        volumeProperty.SetColor(1, color2)
        volumeProperty.SetColor(2, color3)
        volumeProperty.SetColor(3, color4)

        iRen.Initialize();
        ren.SetBackground(0.1,0.4,0.2)
        ren.ResetCamera()
        renWin.Render()

        img_file = "TestGPURayCastIndependentComponentMIP.png"
        svtk.test.Testing.compareImage(
          iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        svtk.test.Testing.interact()
        #iRen.Start()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestGPURayCastIndependentComponentMIP, 'test')])
