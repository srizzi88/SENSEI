/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor3D.cxx

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
#include "svtkDoubleArray.h"
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

#include <algorithm>

//----------------------------------------------------------------------------
int TestLabeledContourMapper(int argc, char* argv[])
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

  // Setup text properties that will be rotated across the isolines:
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

  svtkNew<svtkTextProperty> tprop4;
  tprop4->ShallowCopy(tprop1);
  tprop4->SetColor(.6, .0, .8);
  tprops->AddItem(tprop4);

  svtkNew<svtkTextProperty> tprop5;
  tprop5->ShallowCopy(tprop1);
  tprop5->SetColor(.0, .0, .9);
  tprops->AddItem(tprop5);

  svtkNew<svtkTextProperty> tprop6;
  tprop6->ShallowCopy(tprop1);
  tprop6->SetColor(.7, .8, .2);
  tprops->AddItem(tprop6);

  // Create a text property mapping that will reverse the coloring:
  double* values = contours->GetValues();
  double* valuesEnd = values + contours->GetNumberOfContours();
  svtkNew<svtkDoubleArray> tpropMapping;
  tpropMapping->SetNumberOfComponents(1);
  tpropMapping->SetNumberOfTuples(valuesEnd - values);
  std::reverse_copy(values, valuesEnd, tpropMapping->Begin());

  svtkNew<svtkLabeledContourMapper> mapper;
  mapper->GetPolyDataMapper()->ScalarVisibilityOff();
  mapper->SetTextProperties(tprops);
  mapper->SetTextPropertyMapping(tpropMapping);
  mapper->SetInputConnection(contourStripper->GetOutputPort());
  mapper->SetSkipDistance(100);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

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
