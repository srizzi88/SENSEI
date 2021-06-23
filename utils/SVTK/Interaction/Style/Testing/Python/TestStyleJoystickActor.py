#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

import sys
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Prevent .pyc files being created.
# Stops the svtk source being polluted
# by .pyc files.
sys.dont_write_bytecode = True

# Load base (spike and test)
import TestStyleBaseSpike
import TestStyleBase

class TestStyleJoystickActor(svtk.test.Testing.svtkTest):

    def testStyleJoystickActor(self):

        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);

        testStyleBaseSpike = TestStyleBaseSpike.StyleBaseSpike(ren, renWin, iRen)

        # Set interactor style
        inStyle = svtk.svtkInteractorStyleSwitch()
        iRen.SetInteractorStyle(inStyle)

        # Switch to Joystick+Actor mode

        iRen.SetKeyEventInformation(0, 0, 'j' , 0)
        iRen.InvokeEvent("CharEvent")

        iRen.SetKeyEventInformation(0, 0, 'a', 0)
        iRen.InvokeEvent("CharEvent")
        # Test style
        testStyleBase = TestStyleBase.TestStyleBase(ren)
        testStyleBase.test_style(inStyle.GetCurrentStyle())

        # render and interact with data

        img_file = "TestStyleJoystickActor.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestStyleJoystickActor, 'test')])
