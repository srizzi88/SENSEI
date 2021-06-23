/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSplineWarp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This tests B-spline image warping

#include "svtkBSplineTransform.h"
#include "svtkImageBSplineCoefficients.h"
#include "svtkImageBSplineInterpolator.h"
#include "svtkImageBlend.h"
#include "svtkImageGridSource.h"
#include "svtkImageMapToColors.h"
#include "svtkImageReslice.h"
#include "svtkImageViewer.h"
#include "svtkLookupTable.h"
#include "svtkPoints.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkThinPlateSplineTransform.h"
#include "svtkTransformToGrid.h"

int TestBSplineWarp(int argc, char* argv[])
{
  // first, create an image that looks like
  // graph paper by combining two image grid
  // sources via svtkImageBlend
  svtkSmartPointer<svtkImageGridSource> imageGrid1 = svtkSmartPointer<svtkImageGridSource>::New();
  imageGrid1->SetGridSpacing(4, 4, 0);
  imageGrid1->SetGridOrigin(0, 0, 0);
  imageGrid1->SetDataExtent(0, 255, 0, 255, 0, 0);
  imageGrid1->SetDataScalarTypeToUnsignedChar();

  svtkSmartPointer<svtkImageGridSource> imageGrid2 = svtkSmartPointer<svtkImageGridSource>::New();
  imageGrid2->SetGridSpacing(16, 16, 0);
  imageGrid2->SetGridOrigin(0, 0, 0);
  imageGrid2->SetDataExtent(0, 255, 0, 255, 0, 0);
  imageGrid2->SetDataScalarTypeToUnsignedChar();

  svtkSmartPointer<svtkLookupTable> table1 = svtkSmartPointer<svtkLookupTable>::New();
  table1->SetTableRange(0, 1);
  table1->SetValueRange(1.0, 0.7);
  table1->SetSaturationRange(0.0, 1.0);
  table1->SetHueRange(0.12, 0.12);
  table1->SetAlphaRange(1.0, 1.0);
  table1->Build();

  svtkSmartPointer<svtkLookupTable> table2 = svtkSmartPointer<svtkLookupTable>::New();
  table2->SetTableRange(0, 1);
  table2->SetValueRange(1.0, 0.0);
  table2->SetSaturationRange(0.0, 0.0);
  table2->SetHueRange(0.0, 0.0);
  table2->SetAlphaRange(0.0, 1.0);
  table2->Build();

  svtkSmartPointer<svtkImageMapToColors> map1 = svtkSmartPointer<svtkImageMapToColors>::New();
  map1->SetInputConnection(imageGrid1->GetOutputPort());
  map1->SetLookupTable(table1);

  svtkSmartPointer<svtkImageMapToColors> map2 = svtkSmartPointer<svtkImageMapToColors>::New();
  map2->SetInputConnection(imageGrid2->GetOutputPort());
  map2->SetLookupTable(table2);

  svtkSmartPointer<svtkImageBlend> blend = svtkSmartPointer<svtkImageBlend>::New();
  blend->AddInputConnection(map1->GetOutputPort());
  blend->AddInputConnection(map2->GetOutputPort());

  // next, create a ThinPlateSpline transform, which
  // will then be used to create the B-spline transform
  svtkSmartPointer<svtkPoints> p1 = svtkSmartPointer<svtkPoints>::New();
  p1->SetNumberOfPoints(8);
  p1->SetPoint(0, 0, 0, 0);
  p1->SetPoint(1, 0, 255, 0);
  p1->SetPoint(2, 255, 0, 0);
  p1->SetPoint(3, 255, 255, 0);
  p1->SetPoint(4, 96, 96, 0);
  p1->SetPoint(5, 96, 159, 0);
  p1->SetPoint(6, 159, 159, 0);
  p1->SetPoint(7, 159, 96, 0);

  svtkSmartPointer<svtkPoints> p2 = svtkSmartPointer<svtkPoints>::New();
  p2->SetNumberOfPoints(8);
  p2->SetPoint(0, 0, 0, 0);
  p2->SetPoint(1, 0, 255, 0);
  p2->SetPoint(2, 255, 0, 0);
  p2->SetPoint(3, 255, 255, 0);
  p2->SetPoint(4, 96, 159, 0);
  p2->SetPoint(5, 159, 159, 0);
  p2->SetPoint(6, 159, 96, 0);
  p2->SetPoint(7, 96, 96, 0);

  svtkSmartPointer<svtkThinPlateSplineTransform> thinPlate =
    svtkSmartPointer<svtkThinPlateSplineTransform>::New();
  thinPlate->SetSourceLandmarks(p2);
  thinPlate->SetTargetLandmarks(p1);
  thinPlate->SetBasisToR2LogR();

  // convert the thin plate spline into a B-spline, by
  // sampling it onto a grid and then computing the
  // B-spline coefficients
  svtkSmartPointer<svtkTransformToGrid> transformToGrid = svtkSmartPointer<svtkTransformToGrid>::New();
  transformToGrid->SetInput(thinPlate);
  transformToGrid->SetGridSpacing(16.0, 16.0, 1.0);
  transformToGrid->SetGridOrigin(0.0, 0.0, 0.0);
  transformToGrid->SetGridExtent(0, 16, 0, 16, 0, 0);

  svtkSmartPointer<svtkImageBSplineCoefficients> grid =
    svtkSmartPointer<svtkImageBSplineCoefficients>::New();
  grid->SetInputConnection(transformToGrid->GetOutputPort());
  grid->UpdateWholeExtent();

  // create the B-spline transform, scale the deformation by
  // half to demonstrate how deformation scaling works
  svtkSmartPointer<svtkBSplineTransform> transform = svtkSmartPointer<svtkBSplineTransform>::New();
  transform->SetCoefficientData(grid->GetOutput());
  transform->SetDisplacementScale(0.5);
  transform->SetBorderModeToZero();

  // invert the transform before passing it to svtkImageReslice
  transform->Inverse();

  // reslice the image through the B-spline transform,
  // using B-spline interpolation and the "Repeat"
  // boundary condition
  svtkSmartPointer<svtkImageBSplineCoefficients> prefilter =
    svtkSmartPointer<svtkImageBSplineCoefficients>::New();
  prefilter->SetInputConnection(blend->GetOutputPort());
  prefilter->SetBorderModeToRepeat();
  prefilter->SetSplineDegree(3);

  svtkSmartPointer<svtkImageBSplineInterpolator> interpolator =
    svtkSmartPointer<svtkImageBSplineInterpolator>::New();
  interpolator->SetSplineDegree(3);

  svtkSmartPointer<svtkImageReslice> reslice = svtkSmartPointer<svtkImageReslice>::New();
  reslice->SetInputConnection(prefilter->GetOutputPort());
  reslice->SetResliceTransform(transform);
  reslice->WrapOn();
  reslice->SetInterpolator(interpolator);
  reslice->SetOutputSpacing(1.0, 1.0, 1.0);
  reslice->SetOutputOrigin(-32.0, -32.0, 0.0);
  reslice->SetOutputExtent(0, 319, 0, 319, 0, 0);

  // set the window/level to 255.0/127.5 to view full range
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkImageViewer> viewer = svtkSmartPointer<svtkImageViewer>::New();
  viewer->SetupInteractor(iren);
  viewer->SetInputConnection(reslice->GetOutputPort());
  viewer->SetColorWindow(255.0);
  viewer->SetColorLevel(127.5);
  viewer->SetZSlice(0);
  viewer->Render();

  svtkRenderWindow* renWin = viewer->GetRenderWindow();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
