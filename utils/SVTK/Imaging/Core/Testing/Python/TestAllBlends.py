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

class TestAllBlends(svtk.test.Testing.svtkTest):

    def testAllBlends(self):

        # This script calculates the luminance of an image

        renWin = svtk.svtkRenderWindow()
        renWin.SetSize(512, 256)

        # Image pipeline

        image1 = svtk.svtkTIFFReader()
        image1.SetFileName(SVTK_DATA_ROOT + "/Data/beach.tif")

        # "beach.tif" image contains ORIENTATION tag which is
        # ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
        # reader parses this tag and sets the internal TIFF image
        # orientation accordingly.  To overwrite this orientation with a svtk
        # convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
        # SetOrientationType method with parameter value of 4.
        image1.SetOrientationType(4)

        image2 = svtk.svtkBMPReader()
        image2.SetFileName(SVTK_DATA_ROOT + "/Data/masonry.bmp")

        # shrink the images to a reasonable size

        color = svtk.svtkImageShrink3D()
        color.SetInputConnection(image1.GetOutputPort())
        color.SetShrinkFactors(2, 2, 1)

        backgroundColor = svtk.svtkImageShrink3D()
        backgroundColor.SetInputConnection(image2.GetOutputPort())
        backgroundColor.SetShrinkFactors(2, 2, 1)

        # create a greyscale version

        luminance = svtk.svtkImageLuminance()
        luminance.SetInputConnection(color.GetOutputPort())

        backgroundLuminance = svtk.svtkImageLuminance()
        backgroundLuminance.SetInputConnection(backgroundColor.GetOutputPort())

        # create an alpha mask

        table = svtk.svtkLookupTable()
        table.SetTableRange(220, 255)
        table.SetValueRange(1, 0)
        table.SetSaturationRange(0, 0)
        table.Build()

        alpha = svtk.svtkImageMapToColors()
        alpha.SetInputConnection(luminance.GetOutputPort())
        alpha.SetLookupTable(table)
        alpha.SetOutputFormatToLuminance()

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
                mapper[bg][fg].SetColorWindow(255)
                mapper[bg][fg].SetColorLevel(127.5)

                actor.update({bg:{fg:svtk.svtkActor2D()}})
                actor[bg][fg].SetMapper(mapper[bg][fg])

                imager.update({bg:{fg:svtk.svtkRenderer()}})
                imager[bg][fg].AddActor2D(actor[bg][fg])
                imager[bg][fg].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)

                renWin.AddRenderer(imager[bg][fg])

                column += 1

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllBlends.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(TestAllBlends, 'test')])
