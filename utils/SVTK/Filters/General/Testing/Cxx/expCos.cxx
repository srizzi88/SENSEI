/*=========================================================================

  Program:   Visualization Toolkit
  Module:    expCos.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// Brute force computation of Bessel functions. Might be better to create a
// filter (or source) object. Might also consider svtkSampleFunction.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkFloatArray.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkWarpScalar.h"

int expCos(int, char*[])
{
  int i, numPts;
  double x[3];
  double r, deriv;

  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // create plane to warp
  svtkSmartPointer<svtkPlaneSource> plane = svtkSmartPointer<svtkPlaneSource>::New();
  plane->SetResolution(300, 300);

  svtkSmartPointer<svtkTransform> transform = svtkSmartPointer<svtkTransform>::New();
  transform->Scale(10.0, 10.0, 1.0);

  svtkSmartPointer<svtkTransformPolyDataFilter> transF =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  transF->SetInputConnection(plane->GetOutputPort());
  transF->SetTransform(transform);
  transF->Update();

  // compute Bessel function and derivatives. This portion could be
  // encapsulated into source or filter object.
  //
  svtkSmartPointer<svtkPolyData> input = transF->GetOutput();
  numPts = input->GetNumberOfPoints();

  svtkSmartPointer<svtkPoints> newPts = svtkSmartPointer<svtkPoints>::New();
  newPts->SetNumberOfPoints(numPts);

  svtkSmartPointer<svtkFloatArray> derivs = svtkSmartPointer<svtkFloatArray>::New();
  derivs->SetNumberOfTuples(numPts);

  svtkSmartPointer<svtkPolyData> bessel = svtkSmartPointer<svtkPolyData>::New();
  bessel->CopyStructure(input);
  bessel->SetPoints(newPts);
  bessel->GetPointData()->SetScalars(derivs);

  for (i = 0; i < numPts; i++)
  {
    input->GetPoint(i, x);
    r = sqrt(static_cast<double>(x[0] * x[0]) + x[1] * x[1]);
    x[2] = exp(-r) * cos(10.0 * r);
    newPts->SetPoint(i, x);
    deriv = -exp(-r) * (cos(10.0 * r) + 10.0 * sin(10.0 * r));
    derivs->SetValue(i, deriv);
  }

  // warp plane
  svtkSmartPointer<svtkWarpScalar> warp = svtkSmartPointer<svtkWarpScalar>::New();
  warp->SetInputData(bessel);
  warp->XYPlaneOn();
  warp->SetScaleFactor(0.5);

  // mapper and actor
  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputConnection(warp->GetOutputPort());
  double tmp[2];
  bessel->GetScalarRange(tmp);
  mapper->SetScalarRange(tmp[0], tmp[1]);

  svtkSmartPointer<svtkActor> carpet = svtkSmartPointer<svtkActor>::New();
  carpet->SetMapper(mapper);

  // assign our actor to the renderer
  ren->AddActor(carpet);
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  // draw the resulting scene
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);
  ren->GetActiveCamera()->Elevation(-55);
  ren->GetActiveCamera()->Azimuth(25);
  ren->ResetCameraClippingRange();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
