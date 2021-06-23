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

class imageMCAll(svtk.test.Testing.svtkTest):

    class Colors(object):
        '''
            Provides some wrappers for using color names.
        '''
        def __init__(self):
            '''
                Define a single instance of the NamedColors class here.
            '''
            self.namedColors = svtk.svtkNamedColors()

        def GetRGBColor(self, colorName):
            '''
                Return the red, green and blue components for a
                color as doubles.
            '''
            rgb = [0.0, 0.0, 0.0] # black
            self.namedColors.GetColorRGB(colorName, rgb)
            return rgb

        def GetRGBAColor(self, colorName):
            '''
                Return the red, green, blue and alpha
                components for a color as doubles.
            '''
            rgba = [0.0, 0.0, 0.0, 1.0] # black
            self.namedColors.GetColor(colorName, rgba)
            return rgba

    def testimageMCAll(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        # create pipeline
        #
        slc = svtk.svtkStructuredPointsReader()
        slc.SetFileName(SVTK_DATA_ROOT + "/Data/ironProt.svtk")

        colors = ["flesh", "banana", "grey", "pink", "carrot", "gainsboro", "tomato", "gold", "thistle", "chocolate"]

        types = ["UnsignedChar", "Char", "Short", "UnsignedShort", "Int", "UnsignedInt", "Long", "UnsignedLong", "Float", "Double"]

        i = 1
        c = 0
        clip = list()
        cast = list()
        iso = list()
        mapper = list()
        actor = list()

        colorWrapper = self.Colors()

        for idx, svtkType in enumerate(types):
            clip.append(svtk.svtkImageClip())
            clip[idx].SetInputConnection(slc.GetOutputPort())
            clip[idx].SetOutputWholeExtent(-1000, 1000, -1000, 1000, i, i + 5)
            i += 5
            cast.append(svtk.svtkImageCast())
            eval('cast[idx].SetOutputScalarTypeTo' + svtkType + '()')
            cast[idx].SetInputConnection(clip[idx].GetOutputPort())
            cast[idx].ClampOverflowOn()

            iso.append(svtk.svtkMarchingContourFilter())
            iso[idx].SetInputConnection(cast[idx].GetOutputPort())
            iso[idx].GenerateValues(1, 30, 30)

            mapper.append(svtk.svtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())
            mapper[idx].ScalarVisibilityOff()

            actor.append(svtk.svtkActor())
            actor[idx].SetMapper(mapper[idx])
        #    actor[idx].Actor.GetProperty().SetDiffuseColor(lindex.colors.c.lindex.colors.c+1.lindex.colors.c+1)
            actor[idx].GetProperty().SetDiffuseColor(colorWrapper.GetRGBColor(colors[c]))
            actor[idx].GetProperty().SetSpecularPower(30)
            actor[idx].GetProperty().SetDiffuse(.7)
            actor[idx].GetProperty().SetSpecular(.5)
            c += 1
            ren.AddActor(actor[idx])


        outline = svtk.svtkOutlineFilter()
        outline.SetInputConnection(slc.GetOutputPort())
        outlineMapper = svtk.svtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())
        outlineActor = svtk.svtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.VisibilityOff()

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(outlineActor)
        ren.SetBackground(0.9, .9, .9)
        ren.ResetCamera()
        ren.GetActiveCamera().SetViewAngle(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Zoom(1.5)
        ren.ResetCameraClippingRange()

        renWin.SetSize(400, 400)

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "imageMCAll.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(imageMCAll, 'test')])
