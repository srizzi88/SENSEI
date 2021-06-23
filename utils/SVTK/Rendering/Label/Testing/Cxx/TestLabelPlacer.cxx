/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of svtkLabelPlacer
// .SECTION Description
// this program tests svtkLabelPlacer which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkImageData.h"
#include "svtkLabelHierarchy.h"
#include "svtkLabelPlacer.h"
#include "svtkLabelSizeCalculator.h"
#include "svtkLabeledDataMapper.h"
#include "svtkPointSetToLabelHierarchy.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty.h"
#include "svtkRectilinearGrid.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStructuredGrid.h"
#include "svtkTextProperty.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLPolyDataReader.h"
#include "svtkXMLPolyDataWriter.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestLabelPlacer(int argc, char* argv[])
{
  int maxLevels = 5;
  int targetLabels = 32;
  double labelRatio = 0.05;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uniform-001371-5x5x5.vtp");
  // int iteratorType = svtkLabelHierarchy::FULL_SORT;
  int iteratorType = svtkLabelHierarchy::QUEUE;
  // int iteratorType = svtkLabelHierarchy::DEPTH_FIRST;
  bool showBounds = false;

  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> sphereMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkActor> sphereActor = svtkSmartPointer<svtkActor>::New();

  sphere->SetRadius(5.0);
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  svtkSmartPointer<svtkLabelSizeCalculator> labelSizeCalculator =
    svtkSmartPointer<svtkLabelSizeCalculator>::New();
  svtkSmartPointer<svtkLabelHierarchy> labelHierarchy = svtkSmartPointer<svtkLabelHierarchy>::New();
  svtkSmartPointer<svtkLabelPlacer> labelPlacer = svtkSmartPointer<svtkLabelPlacer>::New();
  svtkSmartPointer<svtkPointSetToLabelHierarchy> pointSetToLabelHierarchy =
    svtkSmartPointer<svtkPointSetToLabelHierarchy>::New();
  svtkSmartPointer<svtkXMLPolyDataReader> xmlPolyDataReader =
    svtkSmartPointer<svtkXMLPolyDataReader>::New();

  svtkSmartPointer<svtkPolyDataMapper> polyDataMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  svtkSmartPointer<svtkLabeledDataMapper> labeledMapper =
    svtkSmartPointer<svtkLabeledDataMapper>::New();
  svtkSmartPointer<svtkActor2D> textActor = svtkSmartPointer<svtkActor2D>::New();

  xmlPolyDataReader->SetFileName(fname);
  delete[] fname;

  labelSizeCalculator->SetInputConnection(xmlPolyDataReader->GetOutputPort());
  labelSizeCalculator->GetFontProperty()->SetFontSize(12);
  labelSizeCalculator->GetFontProperty()->SetFontFamily(
    svtkTextProperty::GetFontFamilyFromString("Arial"));
  labelSizeCalculator->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames");
  labelSizeCalculator->SetLabelSizeArrayName("LabelSize");

  pointSetToLabelHierarchy->AddInputConnection(labelSizeCalculator->GetOutputPort());
  pointSetToLabelHierarchy->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority");
  pointSetToLabelHierarchy->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize");
  pointSetToLabelHierarchy->SetInputArrayToProcess(
    2, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "PlaceNames");
  pointSetToLabelHierarchy->SetMaximumDepth(maxLevels);
  pointSetToLabelHierarchy->SetTargetLabelCount(targetLabels);

  labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
  labelPlacer->SetIteratorType(iteratorType);
  labelPlacer->SetOutputTraversedBounds(showBounds);
  labelPlacer->SetRenderer(renderer);
  labelPlacer->SetMaximumLabelFraction(labelRatio);
  labelPlacer->UseDepthBufferOn();

  polyDataMapper->SetInputConnection(labelPlacer->GetOutputPort());

  actor->SetMapper(polyDataMapper);

  // labelPlacer->Update();

  labeledMapper->SetInputConnection(labelPlacer->GetOutputPort());
  labeledMapper->SetLabelTextProperty(labelSizeCalculator->GetFontProperty());
  labeledMapper->SetFieldDataName("LabelText");
  labeledMapper->SetLabelModeToLabelFieldData();
  labeledMapper->GetLabelTextProperty()->SetColor(0.0, 0.8, 0.2);
  textActor->SetMapper(labeledMapper);

  // renderer->AddActor(actor);
  renderer->AddActor(sphereActor);
  renderer->AddActor(textActor);

  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  iren->SetRenderWindow(renWin);

  renWin->Render();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
