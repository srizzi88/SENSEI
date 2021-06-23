#!/usr/bin/env python
# -*- coding: utf-8 -*-

import math

import svtk
import svtk.test.Testing

# ------------------------------------------------------------
# Purpose: Test more parametric functions.
# ------------------------------------------------------------

class TestMoreParametricFunctions(svtk.test.Testing.svtkTest):
    def testMoreParametricFunctions(self):
        # ------------------------------------------------------------
        # For each parametric surface:
        # 1) Create it
        # 2) Assign mappers and actors
        # 3) Position the object
        # 5) Add a label
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # Create Kuen's Surface.
        # ------------------------------------------------------------
        kuen = svtk.svtkParametricKuen()
        kuenSource = svtk.svtkParametricFunctionSource()
        kuenSource.SetParametricFunction(kuen)
        kuenSource.SetScalarModeToU()

        kuenMapper = svtk.svtkPolyDataMapper()
        kuenMapper.SetInputConnection(kuenSource.GetOutputPort())

        kuenActor = svtk.svtkActor()
        kuenActor.SetMapper(kuenMapper)
        kuenActor.SetPosition(0, -19, 0)
        kuenActor.RotateX(90)

        kuenTextMapper = svtk.svtkTextMapper()
        kuenTextMapper.SetInput("Kuen")
        kuenTextMapper.GetTextProperty().SetJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        kuenTextMapper.GetTextProperty().SetColor(1, 0, 0)
        kuenTextMapper.GetTextProperty().SetFontSize(14)
        kuenTextActor = svtk.svtkActor2D()
        kuenTextActor.SetMapper(kuenTextMapper)
        kuenTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        kuenTextActor.GetPositionCoordinate().SetValue(0, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Pseudosphere
        # ------------------------------------------------------------
        pseudo = svtk.svtkParametricPseudosphere()
        pseudo.SetMinimumU(-3)
        pseudo.SetMaximumU(3)
        pseudoSource = svtk.svtkParametricFunctionSource()
        pseudoSource.SetParametricFunction(pseudo)
        pseudoSource.SetScalarModeToY()

        pseudoMapper = svtk.svtkPolyDataMapper()
        pseudoMapper.SetInputConnection(pseudoSource.GetOutputPort())

        pseudoActor = svtk.svtkActor()
        pseudoActor.SetMapper(pseudoMapper)
        pseudoActor.SetPosition(8, -19, 0)
        pseudoActor.RotateX(90)

        pseudoTextMapper = svtk.svtkTextMapper()
        pseudoTextMapper.SetInput("Pseudosphere")
        pseudoTextMapper.GetTextProperty().SetJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pseudoTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pseudoTextMapper.GetTextProperty().SetFontSize(14)
        pseudoTextActor = svtk.svtkActor2D()
        pseudoTextActor.SetMapper(pseudoTextMapper)
        pseudoTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pseudoTextActor.GetPositionCoordinate().SetValue(8, -22.5, 0)

        # ------------------------------------------------------------
        # Create a Bohemian Dome
        # ------------------------------------------------------------
        bdome = svtk.svtkParametricBohemianDome()
        bdomeSource = svtk.svtkParametricFunctionSource()
        bdomeSource.SetParametricFunction(bdome)
        bdomeSource.SetScalarModeToU()

        bdomeMapper = svtk.svtkPolyDataMapper()
        bdomeMapper.SetInputConnection(bdomeSource.GetOutputPort())

        bdomeActor = svtk.svtkActor()
        bdomeActor.SetMapper(bdomeMapper)
        bdomeActor.SetPosition(16, -19, 0)
        bdomeActor.RotateY(90)

        bdomeTextMapper = svtk.svtkTextMapper()
        bdomeTextMapper.SetInput("Bohemian Dome")
        bdomeTextMapper.GetTextProperty().SetJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bdomeTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bdomeTextMapper.GetTextProperty().SetFontSize(14)
        bdomeTextActor = svtk.svtkActor2D()
        bdomeTextActor.SetMapper(bdomeTextMapper)
        bdomeTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bdomeTextActor.GetPositionCoordinate().SetValue(16, -22.5, 0)

        # ------------------------------------------------------------
        # Create Henneberg's Minimal Surface
        # ------------------------------------------------------------
        hberg = svtk.svtkParametricHenneberg()
        hberg.SetMinimumU(-.3)
        hberg.SetMaximumU(.3)
        hbergSource = svtk.svtkParametricFunctionSource()
        hbergSource.SetParametricFunction(hberg)
        hbergSource.SetScalarModeToV()

        hbergMapper = svtk.svtkPolyDataMapper()
        hbergMapper.SetInputConnection(hbergSource.GetOutputPort())

        hbergActor = svtk.svtkActor()
        hbergActor.SetMapper(hbergMapper)
        hbergActor.SetPosition(24, -19, 0)
        hbergActor.RotateY(90)

        hbergTextMapper = svtk.svtkTextMapper()
        hbergTextMapper.SetInput("Henneberg")
        hbergTextMapper.GetTextProperty().SetJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        hbergTextMapper.GetTextProperty().SetColor(1, 0, 0)
        hbergTextMapper.GetTextProperty().SetFontSize(14)
        hbergTextActor = svtk.svtkActor2D()
        hbergTextActor.SetMapper(hbergTextMapper)
        hbergTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        hbergTextActor.GetPositionCoordinate().SetValue(24, -22.5, 0)

        # ------------------------------------------------------------
        # Create Catalan's Minimal Surface
        # ------------------------------------------------------------
        catalan = svtk.svtkParametricCatalanMinimal()
        catalan.SetMinimumU(-2.*math.pi)
        catalan.SetMaximumU( 2.*math.pi)
        catalanSource = svtk.svtkParametricFunctionSource()
        catalanSource.SetParametricFunction(catalan)
        catalanSource.SetScalarModeToV()

        catalanMapper = svtk.svtkPolyDataMapper()
        catalanMapper.SetInputConnection(catalanSource.GetOutputPort())

        catalanActor = svtk.svtkActor()
        catalanActor.SetMapper(catalanMapper)
        catalanActor.SetPosition(0, -27, 0)
        catalanActor.SetScale(.5, .5, .5)

        catalanTextMapper = svtk.svtkTextMapper()
        catalanTextMapper.SetInput("Catalan")
        catalanTextMapper.GetTextProperty().SetJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        catalanTextMapper.GetTextProperty().SetColor(1, 0, 0)
        catalanTextMapper.GetTextProperty().SetFontSize(14)
        catalanTextActor = svtk.svtkActor2D()
        catalanTextActor.SetMapper(catalanTextMapper)
        catalanTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        catalanTextActor.GetPositionCoordinate().SetValue(0, -30.5, 0)

        # ------------------------------------------------------------
        # Create Bour's Minimal Surface
        # ------------------------------------------------------------
        bour = svtk.svtkParametricBour()
        bourSource = svtk.svtkParametricFunctionSource()
        bourSource.SetParametricFunction(bour)
        bourSource.SetScalarModeToU()

        bourMapper = svtk.svtkPolyDataMapper()
        bourMapper.SetInputConnection(bourSource.GetOutputPort())

        bourActor = svtk.svtkActor()
        bourActor.SetMapper(bourMapper)
        bourActor.SetPosition(8, -27, 0)

        bourTextMapper = svtk.svtkTextMapper()
        bourTextMapper.SetInput("Bour")
        bourTextMapper.GetTextProperty().SetJustificationToCentered()
        bourTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        bourTextMapper.GetTextProperty().SetColor(1, 0, 0)
        bourTextMapper.GetTextProperty().SetFontSize(14)
        bourTextActor = svtk.svtkActor2D()
        bourTextActor.SetMapper(bourTextMapper)
        bourTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        bourTextActor.GetPositionCoordinate().SetValue(8, -30.5, 0)

        # ------------------------------------------------------------
        # Create Plucker's Conoid Surface
        # ------------------------------------------------------------
        plucker = svtk.svtkParametricPluckerConoid()
        pluckerSource = svtk.svtkParametricFunctionSource()
        pluckerSource.SetParametricFunction(plucker)
        pluckerSource.SetScalarModeToZ()

        pluckerMapper = svtk.svtkPolyDataMapper()
        pluckerMapper.SetInputConnection(pluckerSource.GetOutputPort())

        pluckerActor = svtk.svtkActor()
        pluckerActor.SetMapper(pluckerMapper)
        pluckerActor.SetPosition(16, -27, 0)

        pluckerTextMapper = svtk.svtkTextMapper()
        pluckerTextMapper.SetInput("Plucker")
        pluckerTextMapper.GetTextProperty().SetJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetVerticalJustificationToCentered()
        pluckerTextMapper.GetTextProperty().SetColor(1, 0, 0)
        pluckerTextMapper.GetTextProperty().SetFontSize(14)
        pluckerTextActor = svtk.svtkActor2D()
        pluckerTextActor.SetMapper(pluckerTextMapper)
        pluckerTextActor.GetPositionCoordinate().SetCoordinateSystemToWorld()
        pluckerTextActor.GetPositionCoordinate().SetValue(16, -30.5, 0)

        # ------------------------------------------------------------
        # Create the RenderWindow, Renderer and all svtkActors
        # ------------------------------------------------------------
        ren = svtk.svtkRenderer()
        renWin = svtk.svtkRenderWindow()
        renWin.AddRenderer(ren)
        iren = svtk.svtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        # add actors
        ren.AddViewProp(kuenActor)
        ren.AddViewProp(pseudoActor)
        ren.AddViewProp(bdomeActor)
        ren.AddViewProp(hbergActor)
        ren.AddViewProp(catalanActor)
        ren.AddViewProp(bourActor)
        ren.AddViewProp(pluckerActor)
        #add text actors
        ren.AddViewProp(kuenTextActor)
        ren.AddViewProp(pseudoTextActor)
        ren.AddViewProp(bdomeTextActor)
        ren.AddViewProp(hbergTextActor)
        ren.AddViewProp(catalanTextActor)
        ren.AddViewProp(bourTextActor)
        ren.AddViewProp(pluckerTextActor)

        ren.SetBackground(0.9, 0.9, 0.9)
        renWin.SetSize(500, 500)
        ren.ResetCamera()

        iren.Initialize()
        renWin.Render()

        img_file = "TestMoreParametricFunctions.png"
        svtk.test.Testing.compareImage(iren.GetRenderWindow(), svtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        svtk.test.Testing.interact()

if __name__ == "__main__":
    svtk.test.Testing.main([(TestMoreParametricFunctions, 'test')])
