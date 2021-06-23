/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAxisActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkAxisActor.h"
#include "svtkCamera.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestAxisActor3D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the axis actor
  svtkSmartPointer<svtkAxisActor> axis = svtkSmartPointer<svtkAxisActor>::New();
  axis->SetPoint1(0, 0, 0);
  axis->SetPoint2(1, 1, 0);
  axis->SetBounds(0, 1, 0, 0, 0, 0);
  axis->SetTickLocationToBoth();
  axis->SetAxisTypeToX();
  axis->SetTitle("1.0");
  axis->SetTitleScale(0.5);
  axis->SetTitleVisibility(1);
  axis->SetMajorTickSize(0.01);
  axis->SetRange(0, 1);

  svtkSmartPointer<svtkStringArray> labels = svtkSmartPointer<svtkStringArray>::New();
  labels->SetNumberOfTuples(1);
  labels->SetValue(0, "X");

  axis->SetLabels(labels);
  axis->SetLabelScale(.2);
  axis->MinorTicksVisibleOff();
  axis->SetDeltaMajor(0, .1);
  axis->SetCalculateTitleOffset(0);
  axis->SetCalculateLabelOffset(0);
  axis->Print(std::cout);

  svtkSmartPointer<svtkSphereSource> source = svtkSmartPointer<svtkSphereSource>::New();
  source->SetCenter(1, 1, 1);
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(source->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Create the RenderWindow, Renderer and both Actors
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  axis->SetCamera(ren1->GetActiveCamera());

  ren1->AddActor(actor);
  ren1->AddActor(axis);

  ren1->SetBackground(.3, .4, .5);
  renWin->SetSize(500, 200);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // render the image
  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
