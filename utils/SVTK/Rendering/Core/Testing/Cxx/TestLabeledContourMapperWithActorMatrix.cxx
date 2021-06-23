/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabeledContourMapperWithActorMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkLabeledContourMapper.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkContourFilter.h"
#include "svtkDEMReader.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStripper.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkTextPropertyCollection.h"
#include "svtkTransform.h"

//----------------------------------------------------------------------------
int TestLabeledContourMapperWithActorMatrix(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  svtkNew<svtkDEMReader> demReader;
  demReader->SetFileName(fname);
  delete[] fname;

  double range[2];
  demReader->Update();
  demReader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  svtkNew<svtkContourFilter> contours;
  contours->SetInputConnection(demReader->GetOutputPort());
  contours->GenerateValues(21, range[0], range[1]);

  svtkNew<svtkStripper> contourStripper;
  contourStripper->SetInputConnection(contours->GetOutputPort());
  contourStripper->Update();

  // Setup three text properties that will be rotated across the isolines:
  svtkNew<svtkTextPropertyCollection> tprops;
  svtkNew<svtkTextProperty> tprop1;
  tprop1->SetBold(1);
  tprop1->SetFontSize(12);
  tprop1->SetBackgroundColor(0.5, 0.5, 0.5);
  tprop1->SetBackgroundOpacity(0.25);
  tprop1->SetColor(1., 1., 1.);
  tprops->AddItem(tprop1);

  svtkNew<svtkTextProperty> tprop2;
  tprop2->ShallowCopy(tprop1);
  tprop2->SetColor(.8, .2, .3);
  tprops->AddItem(tprop2);

  svtkNew<svtkTextProperty> tprop3;
  tprop3->ShallowCopy(tprop1);
  tprop3->SetColor(.3, .8, .2);
  tprops->AddItem(tprop3);

  svtkNew<svtkLabeledContourMapper> mapper;
  mapper->GetPolyDataMapper()->ScalarVisibilityOff();
  mapper->SetTextProperties(tprops);
  mapper->SetInputConnection(contourStripper->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkTransform> xform;
  xform->Identity();
  xform->Scale(0.5, 0.25, 10.);
  xform->RotateWXYZ(196, 0.0, 0.0, 1.0);
  xform->Translate(50, 50, 50);
  actor->SetUserTransform(xform);

  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);

  svtkNew<svtkRenderWindow> win;
  win->SetStencilCapable(1); // Needed for svtkLabeledContourMapper
  win->AddRenderer(ren);

  double bounds[6];
  contourStripper->GetOutput()->GetBounds(bounds);

  win->SetSize(600, 600);
  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->GetActiveCamera()->SetPosition(
    (bounds[0] + bounds[1]) / 2., (bounds[2] + bounds[3]) / 2., 0);

  ren->GetActiveCamera()->SetFocalPoint(
    (bounds[0] + bounds[1]) / 2., (bounds[2] + bounds[3]) / 2., (bounds[4] + bounds[5]) / 2.);
  ren->ResetCamera();
  ren->GetActiveCamera()->Dolly(6.5);
  ren->ResetCameraClippingRange();

  win->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
