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

import StringIO
import sys
import svtk
import svtk.test.Testing
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

class MassProperties(svtk.test.Testing.svtkTest):

    def testMassProperties(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)

        cone = svtk.svtkConeSource()
        cone.SetResolution(50)

        sphere = svtk.svtkSphereSource()
        sphere.SetPhiResolution(50)
        sphere.SetThetaResolution(50)

        cube = svtk.svtkCubeSource()
        cube.SetXLength(1)
        cube.SetYLength(1)
        cube.SetZLength(1)

        sphereMapper = svtk.svtkPolyDataMapper()
        sphereMapper.SetInputConnection(sphere.GetOutputPort())

        sphereActor = svtk.svtkActor()
        sphereActor.SetMapper(sphereMapper)
        sphereActor.GetProperty().SetDiffuseColor(1, .2, .4)
        coneMapper = svtk.svtkPolyDataMapper()
        coneMapper.SetInputConnection(cone.GetOutputPort())

        coneActor = svtk.svtkActor()
        coneActor.SetMapper(coneMapper)
        coneActor.GetProperty().SetDiffuseColor(.2, .4, 1)

        cubeMapper = svtk.svtkPolyDataMapper()
        cubeMapper.SetInputConnection(cube.GetOutputPort())

        cubeActor = svtk.svtkActor()
        cubeActor.SetMapper(cubeMapper)
        cubeActor.GetProperty().SetDiffuseColor(.2, 1, .4)

        #Add the actors to the renderer, set the background and size
        #
        sphereActor.SetPosition(-5, 0, 0)
        ren.AddActor(sphereActor)
        coneActor.SetPosition(0, 0, 0)
        ren.AddActor(coneActor)
        coneActor.SetPosition(5, 0, 0)
        ren.AddActor(cubeActor)

        tf = dict()
        mp = dict()
        vt = dict()
        pdm = dict()
        ta = dict()

        def MakeText(primitive):

            tf.update({primitive: svtk.svtkTriangleFilter()})
            tf[primitive].SetInputConnection(primitive.GetOutputPort())

            mp.update({primitive: svtk.svtkMassProperties()})
            mp[primitive].SetInputConnection(tf[primitive].GetOutputPort())

            # here we capture stdout and write it to a variable for processing.
            summary = StringIO.StringIO()
            # save the original stdout
            old_stdout = sys.stdout
            sys.stdout = summary

            print mp[primitive]
            summary = summary.getvalue()

            startSum = summary.find("  VolumeX")
            endSum = len(summary)
            print summary[startSum:]
            # Restore stdout
            sys.stdout = old_stdout

            vt.update({primitive: svtk.svtkVectorText()})
            vt[primitive].SetText(summary[startSum:])

            pdm.update({primitive: svtk.svtkPolyDataMapper()})
            pdm[primitive].SetInputConnection(vt[primitive].GetOutputPort())

            ta.update({primitive: svtk.svtkActor()})
            ta[primitive].SetMapper(pdm[primitive])
            ta[primitive].SetScale(.2, .2, .2)
            return ta[primitive]


        ren.AddActor(MakeText(sphere))
        ren.AddActor(MakeText(cube))
        ren.AddActor(MakeText(cone))

        ta[sphere].SetPosition(sphereActor.GetPosition())
        ta[sphere].AddPosition(-2, -1, 0)
        ta[cube].SetPosition(cubeActor.GetPosition())
        ta[cube].AddPosition(-2, -1, 0)
        ta[cone].SetPosition(coneActor.GetPosition())
        ta[cone].AddPosition(-2, -1, 0)

        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(786, 256)

        # render the image
        #
        ren.ResetCamera()
        cam1 = ren.GetActiveCamera()
        cam1.Dolly(3)
        ren.ResetCameraClippingRange()

        # render and interact with data

        iRen = svtk.svtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "MassProperties.png"
        svtk.test.Testing.compareImage(iRen.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        svtk.test.Testing.interact()

if __name__ == "__main__":
     svtk.test.Testing.main([(MassProperties, 'test')])
