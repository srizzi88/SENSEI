/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPropItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPropItem.h"

#include "svtkActor.h"
#include "svtkAxis.h"
#include "svtkBoundingBox.h"
#include "svtkContextArea.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkContourFilter.h"
#include "svtkDEMReader.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkLabeledContourMapper.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPen.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRect.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStripper.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestPropItem(int argc, char* argv[])
{
  // Prepare some data for plotting:
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  svtkNew<svtkDEMReader> demReader;
  demReader->SetFileName(fname);
  delete[] fname;

  // Get dataset metadata:
  demReader->Update();
  svtkBoundingBox bounds(demReader->GetOutput()->GetBounds());
  double scalarRange[2];
  demReader->GetOutput()->GetScalarRange(scalarRange);

  // Raw data:
  svtkNew<svtkImageDataGeometryFilter> imageToPd;
  imageToPd->SetInputConnection(demReader->GetOutputPort());

  svtkNew<svtkPolyDataMapper> imageMapper;
  imageMapper->SetInputConnection(imageToPd->GetOutputPort());
  imageMapper->SetScalarVisibility(1);

  svtkNew<svtkLookupTable> imageLUT;
  imageLUT->SetHueRange(0.6, 0);
  imageLUT->SetSaturationRange(1.0, 0.25);
  imageLUT->SetValueRange(0.5, 1.0);

  imageMapper->SetLookupTable(imageLUT);
  imageMapper->SetScalarRange(scalarRange);

  svtkNew<svtkActor> imageActor;
  imageActor->SetMapper(imageMapper);

  svtkNew<svtkPropItem> imageItem;
  imageItem->SetPropObject(imageActor);

  // Contours:
  double range[2];
  demReader->Update();
  demReader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  svtkNew<svtkContourFilter> contours;
  contours->SetInputConnection(demReader->GetOutputPort());
  contours->GenerateValues(21, range[0], range[1]);

  svtkNew<svtkStripper> contourStripper;
  contourStripper->SetInputConnection(contours->GetOutputPort());

  svtkNew<svtkLabeledContourMapper> contourMapper;
  contourMapper->SetInputConnection(contourStripper->GetOutputPort());

  svtkNew<svtkTextProperty> tprop;
  tprop->SetBold(1);
  tprop->SetFontSize(12);
  tprop->SetColor(1., 1., 1.);
  contourMapper->SetTextProperty(tprop);

  svtkNew<svtkLookupTable> contourLUT;
  contourLUT->SetHueRange(0.6, 0);
  contourLUT->SetSaturationRange(0.75, 1.0);
  contourLUT->SetValueRange(0.25, 0.75);

  contourMapper->GetPolyDataMapper()->SetLookupTable(contourLUT);
  contourMapper->GetPolyDataMapper()->SetScalarRange(scalarRange);

  svtkNew<svtkActor> contourActor;
  contourActor->SetMapper(contourMapper);

  svtkNew<svtkPropItem> contourItem;
  contourItem->SetPropObject(contourActor);

  //----------------------------------------------------------------------------
  // Context2D initialization:

  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(0.2, 0.2, 0.7);
  view->GetRenderWindow()->SetSize(600, 600);
  view->GetRenderWindow()->StencilCapableOn(); // For svtkLabeledContourMapper
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  svtkNew<svtkContextArea> area;
  area->ShowGridOff();
  area->SetDrawAreaBounds(
    svtkRectd(bounds.GetBound(0), bounds.GetBound(2), bounds.GetLength(0), bounds.GetLength(1)));

  area->SetFixedAspect(bounds.GetLength(0) / bounds.GetLength(1));

  area->GetAxis(svtkAxis::TOP)->SetTitle("Top Axis");
  area->GetAxis(svtkAxis::BOTTOM)->SetTitle("Bottom Axis");
  area->GetAxis(svtkAxis::LEFT)->SetTitle("Left Axis");
  area->GetAxis(svtkAxis::RIGHT)->SetTitle("Right Axis");

  for (int i = 0; i < 4; ++i)
  {
    svtkAxis* axis = area->GetAxis(static_cast<svtkAxis::Location>(i));
    axis->GetLabelProperties()->SetColor(.6, .6, .9);
    axis->GetTitleProperties()->SetColor(.6, .6, .9);
    axis->GetPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 255);
    axis->GetGridPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 128);
  }

  area->GetDrawAreaItem()->AddItem(imageItem);
  area->GetDrawAreaItem()->AddItem(contourItem);

  view->GetScene()->AddItem(area);

  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
