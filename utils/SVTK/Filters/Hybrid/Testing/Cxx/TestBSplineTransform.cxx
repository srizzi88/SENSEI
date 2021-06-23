/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSplineTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  This test builds a thin-plate spline transform, and then approximates
  it with a B-Spline transform.  It applies both the B-Spline transform
  and the original thin-plate spline transform to a polydata so that they
  can be compared.

  The output image is displayed as eight separate panels, as follows:

  Top row:
    1) thin-plate spline applied to a sphere
    2) B-spline applied to a sphere
    3) thin-plate spline applied to a sphere with normals
    4) B-spline applied to a sphere with normals
  Bottom row:
    Same as top row, but with inverted transform
*/

#include <svtkActor.h>
#include <svtkBSplineTransform.h>
#include <svtkImageBSplineCoefficients.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkThinPlateSplineTransform.h>
#include <svtkTransformPolyDataFilter.h>
#include <svtkTransformToGrid.h>

int TestBSplineTransform(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(600, 300);

  // Make a sphere
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);
  sphere->Update();

  // Make another sphere, with no normals
  svtkSmartPointer<svtkPolyData> sphereData = svtkSmartPointer<svtkPolyData>::New();
  sphereData->SetPoints(sphere->GetOutput()->GetPoints());
  sphereData->SetPolys(sphere->GetOutput()->GetPolys());

  // ---------------------------
  // start with a thin plate spline transform
  svtkSmartPointer<svtkPoints> spoints = svtkSmartPointer<svtkPoints>::New();
  spoints->SetNumberOfPoints(10);
  spoints->SetPoint(0, 0.000, 0.000, 0.500);
  spoints->SetPoint(1, 0.000, 0.000, -0.500);
  spoints->SetPoint(2, 0.433, 0.000, 0.250);
  spoints->SetPoint(3, 0.433, 0.000, -0.250);
  spoints->SetPoint(4, -0.000, 0.433, 0.250);
  spoints->SetPoint(5, -0.000, 0.433, -0.250);
  spoints->SetPoint(6, -0.433, -0.000, 0.250);
  spoints->SetPoint(7, -0.433, -0.000, -0.250);
  spoints->SetPoint(8, 0.000, -0.433, 0.250);
  spoints->SetPoint(9, 0.000, -0.433, -0.250);

  svtkSmartPointer<svtkPoints> tpoints = svtkSmartPointer<svtkPoints>::New();
  tpoints->SetNumberOfPoints(10);
  tpoints->SetPoint(0, 0.000, 0.000, 0.800);
  tpoints->SetPoint(1, 0.000, 0.000, -0.200);
  tpoints->SetPoint(2, 0.433, 0.000, 0.350);
  tpoints->SetPoint(3, 0.433, 0.000, -0.150);
  tpoints->SetPoint(4, -0.000, 0.233, 0.350);
  tpoints->SetPoint(5, -0.000, 0.433, -0.150);
  tpoints->SetPoint(6, -0.433, -0.000, 0.350);
  tpoints->SetPoint(7, -0.433, -0.000, -0.150);
  tpoints->SetPoint(8, 0.000, -0.233, 0.350);
  tpoints->SetPoint(9, 0.000, -0.433, -0.150);

  svtkSmartPointer<svtkThinPlateSplineTransform> thin =
    svtkSmartPointer<svtkThinPlateSplineTransform>::New();
  thin->SetSourceLandmarks(spoints);
  thin->SetTargetLandmarks(tpoints);
  thin->SetBasisToR2LogR();

  // First pane: thin-plate, no normals
  svtkSmartPointer<svtkTransformPolyDataFilter> f11 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f11->SetInputData(sphereData);
  f11->SetTransform(thin);

  svtkSmartPointer<svtkPolyDataMapper> m11 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m11->SetInputConnection(f11->GetOutputPort());

  svtkSmartPointer<svtkActor> a11 = svtkSmartPointer<svtkActor>::New();
  a11->SetMapper(m11);
  a11->RotateY(90);
  a11->GetProperty()->SetColor(1, 0, 0);

  svtkSmartPointer<svtkRenderer> ren11 = svtkSmartPointer<svtkRenderer>::New();
  ren11->SetViewport(0.0, 0.5, 0.25, 1.0);
  ren11->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren11->AddActor(a11);
  renWin->AddRenderer(ren11);

  // Invert the transform
  svtkSmartPointer<svtkTransformPolyDataFilter> f12 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f12->SetInputData(sphereData);
  f12->SetTransform(thin->GetInverse());

  svtkSmartPointer<svtkPolyDataMapper> m12 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m12->SetInputConnection(f12->GetOutputPort());

  svtkSmartPointer<svtkActor> a12 = svtkSmartPointer<svtkActor>::New();
  a12->SetMapper(m12);
  a12->RotateY(90);
  a12->GetProperty()->SetColor(0.9, 0.9, 0);

  svtkSmartPointer<svtkRenderer> ren12 = svtkSmartPointer<svtkRenderer>::New();
  ren12->SetViewport(0.0, 0.0, 0.25, 0.5);
  ren12->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren12->AddActor(a12);
  renWin->AddRenderer(ren12);

  // Second pane: b-spline transform, no normals
  svtkSmartPointer<svtkTransformToGrid> transformToGrid = svtkSmartPointer<svtkTransformToGrid>::New();
  transformToGrid->SetInput(thin);
  transformToGrid->SetGridOrigin(-1.5, -1.5, -1.5);
  transformToGrid->SetGridExtent(0, 60, 0, 60, 0, 60);
  transformToGrid->SetGridSpacing(0.05, 0.05, 0.05);

  svtkSmartPointer<svtkImageBSplineCoefficients> coeffs =
    svtkSmartPointer<svtkImageBSplineCoefficients>::New();
  coeffs->SetInputConnection(transformToGrid->GetOutputPort());

  svtkSmartPointer<svtkBSplineTransform> t2 = svtkSmartPointer<svtkBSplineTransform>::New();
  t2->SetCoefficientConnection(coeffs->GetOutputPort());

  svtkSmartPointer<svtkTransformPolyDataFilter> f21 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f21->SetInputData(sphereData);
  f21->SetTransform(t2);

  svtkSmartPointer<svtkPolyDataMapper> m21 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m21->SetInputConnection(f21->GetOutputPort());

  svtkSmartPointer<svtkActor> a21 = svtkSmartPointer<svtkActor>::New();
  a21->SetMapper(m21);
  a21->RotateY(90);
  a21->GetProperty()->SetColor(1, 0, 0);

  svtkSmartPointer<svtkRenderer> ren21 = svtkSmartPointer<svtkRenderer>::New();
  ren21->SetViewport(0.25, 0.5, 0.50, 1.0);
  ren21->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren21->AddActor(a21);
  renWin->AddRenderer(ren21);

  // Invert the transform
  svtkSmartPointer<svtkTransformPolyDataFilter> f22 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f22->SetInputData(sphereData);
  f22->SetTransform(t2->GetInverse());

  svtkSmartPointer<svtkPolyDataMapper> m22 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m22->SetInputConnection(f22->GetOutputPort());

  svtkSmartPointer<svtkActor> a22 = svtkSmartPointer<svtkActor>::New();
  a22->SetMapper(m22);
  a22->RotateY(90);
  a22->GetProperty()->SetColor(0.9, 0.9, 0);

  svtkSmartPointer<svtkRenderer> ren22 = svtkSmartPointer<svtkRenderer>::New();
  ren22->SetViewport(0.25, 0.0, 0.50, 0.5);
  ren22->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren22->AddActor(a22);
  renWin->AddRenderer(ren22);

  // Third pane: thin-plate, no normals
  svtkSmartPointer<svtkTransformPolyDataFilter> f31 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f31->SetInputConnection(sphere->GetOutputPort());
  f31->SetTransform(thin);

  svtkSmartPointer<svtkPolyDataMapper> m31 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m31->SetInputConnection(f31->GetOutputPort());

  svtkSmartPointer<svtkActor> a31 = svtkSmartPointer<svtkActor>::New();
  a31->SetMapper(m31);
  a31->RotateY(90);
  a31->GetProperty()->SetColor(1, 0, 0);

  svtkSmartPointer<svtkRenderer> ren31 = svtkSmartPointer<svtkRenderer>::New();
  ren31->SetViewport(0.50, 0.5, 0.75, 1.0);
  ren31->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren31->AddActor(a31);
  renWin->AddRenderer(ren31);

  // Invert the transform
  svtkSmartPointer<svtkTransformPolyDataFilter> f32 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f32->SetInputConnection(sphere->GetOutputPort());
  f32->SetTransform(thin->GetInverse());

  svtkSmartPointer<svtkPolyDataMapper> m32 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m32->SetInputConnection(f32->GetOutputPort());

  svtkSmartPointer<svtkActor> a32 = svtkSmartPointer<svtkActor>::New();
  a32->SetMapper(m32);
  a32->RotateY(90);
  a32->GetProperty()->SetColor(0.9, 0.9, 0);

  svtkSmartPointer<svtkRenderer> ren32 = svtkSmartPointer<svtkRenderer>::New();
  ren32->SetViewport(0.5, 0.0, 0.75, 0.5);
  ren32->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren32->AddActor(a32);
  renWin->AddRenderer(ren32);

  // Third pane: b-spline, normals
  svtkSmartPointer<svtkBSplineTransform> t4 = svtkSmartPointer<svtkBSplineTransform>::New();
  t4->SetCoefficientConnection(coeffs->GetOutputPort());

  svtkSmartPointer<svtkTransformPolyDataFilter> f41 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f41->SetInputConnection(sphere->GetOutputPort());
  f41->SetTransform(t4);

  svtkSmartPointer<svtkPolyDataMapper> m41 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m41->SetInputConnection(f41->GetOutputPort());

  svtkSmartPointer<svtkActor> a41 = svtkSmartPointer<svtkActor>::New();
  a41->SetMapper(m41);
  a41->RotateY(90);
  a41->GetProperty()->SetColor(1, 0, 0);

  svtkSmartPointer<svtkRenderer> ren41 = svtkSmartPointer<svtkRenderer>::New();
  ren41->SetViewport(0.75, 0.5, 1.0, 1.0);
  ren41->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren41->AddActor(a41);
  renWin->AddRenderer(ren41);

  // Invert the transform
  svtkSmartPointer<svtkTransformPolyDataFilter> f42 =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  f42->SetInputConnection(sphere->GetOutputPort());
  f42->SetTransform(t4->GetInverse());

  svtkSmartPointer<svtkPolyDataMapper> m42 = svtkSmartPointer<svtkPolyDataMapper>::New();
  m42->SetInputConnection(f42->GetOutputPort());

  svtkSmartPointer<svtkActor> a42 = svtkSmartPointer<svtkActor>::New();
  a42->SetMapper(m42);
  a42->RotateY(90);
  a42->GetProperty()->SetColor(0.9, 0.9, 0);

  svtkSmartPointer<svtkRenderer> ren42 = svtkSmartPointer<svtkRenderer>::New();
  ren42->SetViewport(0.75, 0.0, 1.0, 0.5);
  ren42->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren42->AddActor(a42);
  renWin->AddRenderer(ren42);

  // you MUST NOT call renderWindow->Render() before
  // iren->SetRenderWindow(renderWindow);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // render and interact
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
