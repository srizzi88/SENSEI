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

import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class TestAllBlendsFloat(svtk.test.Testing.svtkTest):

    def testAllBlendsFloat(self):

        # This script blends images that consist of float data

        renWin = svtk.svtkRenderWindow()
        renWin.SetSize(512, 256)

        # Image pipeline

        inputImage = svtk.svtkTIFFReader()
        inputImage.SetFileName(SVTK_DATA_ROOT + "/Data/beach.tif")

        # "beach.tif" image contains ORIENTATION tag which is
        # ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
        # reader parses this tag and sets the internal TIFF image
        # orientation accordingly.  To overwrite this orientation with a svtk
        # convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
        # SetOrientationType method with parameter value of 4.
        inputImage.SetOrientationType(4)

        inputImage2 = svtk.svtkBMPReader()
        inputImage2.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")

        # shrink the images to a reasonable size

        shrink1 = svtk.svtkImageShrink3D()
        shrink1.SetInputConnection(inputImage.GetOutputPort())
        shrink1.SetShrinkFactors(2, 2, 1)

        shrink2 = svtk.svtkImageShrink3D()
        shrink2.SetInputConnection(inputImage2.GetOutputPort())
        shrink2.SetShrinkFactors(2, 2, 1)

        color = svtk.svtkImageShiftScale()
        color.SetOutputScalarTypeToFloat()
        color.SetShift(0)
        color.SetScale(1.0 / 255)
        color.SetInputConnection(shrink1.GetOutputPort())

        backgroundColor = svtk.svtkImageShiftScale()
        backgroundColor.SetOutputScalarTypeToFloat()
        backgroundColor.SetShift(0)
        backgroundColor.SetScale(1.0 / 255)
        backgroundColor.SetInputConnection(shrink2.GetOutputPort())

        # create a greyscale version

        luminance = svtk.svtkImageLuminance()
        luminance.SetInputConnection(color.GetOutputPort())

        backgroundLuminance = svtk.svtkImageLuminance()
        backgroundLuminance.SetInputConnection(backgroundColor.GetOutputPort())

        # create an alpha mask

        alpha = svtk.svtkImageThreshold()
        alpha.SetInputConnection(luminance.GetOutputPort())
        alpha.ThresholdByLower(0.9)
        alpha.SetInValue(1.0)
        alpha.SetOutValue(0.0)

        # make luminanceAlpha and colorAlpha versions

        luminanceAlpha = svtk.svtkImageAppendComponents()
        luminanceAlpha.AddInputConnection(luminance.GetOutputPort())
        luminanceAlpha.AddInputConnection(alpha.GetOutputPort())

        colorAlpha = svtk.svtkImageAppendComponents()
        colorAlpha.AddInputConnection(color.GetOutputPort())
        colorAlpha.AddInputConnection(alpha.GetOutputPort())

        foregrounds = ["luminance", "luminanceAlpha", "color", "colorAlpha"]
        backgrounds = ["backgroundColor", "backgroundLuminance"]

        deltaX = 1.0 / 4.0
        deltaY = 1.0 / 2.0

        blend = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for row, bg in enumerate(backgrounds):
            for column, fg in enumerate(foregrounds):
                blend.update({bg:{fg:svtk.svtkImageBlend()}})
                blend[bg][fg].AddInputConnection(eval(bg + '.GetOutputPort()'))
                if bg == "backgroundColor" or fg == "luminance" or fg == "luminanceAlpha":
                    blend[bg][fg].AddInputConnection(eval(fg + '.GetOutputPort()'))
                    blend[bg][fg].SetOpacity(1, 0.8)

                mapper.update({bg:{fg:svtk.svtkImageMapper()}})
                mapper[bg][fg].SetInputConnection(blend[bg][fg].GetOutputPort())
                mapper[bg][fg].SetColorWindow(1.0)
                mapper[bg][fg].SetColorLevel(0.5)

                actor.update({bg:{fg:svtk.svtkActor2D()}})
                actor[bg][fg].SetMapper(mapper[bg][fg])

                imager.update({bg:{fg:svtk.svtkRenderer()}})
                imager[bg][fg].AddActor2D(actor[bg][fg])
                imager[bg][fg].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)

                renWin.AddRenderer(imager[bg][fg])

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllBlendsFloat.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestAllBlendsFloat, 'test')])
