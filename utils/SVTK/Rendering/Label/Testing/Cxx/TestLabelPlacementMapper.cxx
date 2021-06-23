/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacementMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of svtkLabelPlacementMapper
// .SECTION Description
// this program tests svtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkInteractorStyleSwitch.h"
#include "svtkLabelHierarchy.h"
#include "svtkLabelPlacementMapper.h"
#include "svtkLabeledDataMapper.h"
#include "svtkPointData.h"
#include "svtkPointSetToLabelHierarchy.h"
#include "svtkPointSource.h"
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
#include "svtkStringArray.h"
#include "svtkStructuredGrid.h"
#include "svtkTextProperty.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLPolyDataReader.h"
#include "svtkXMLPolyDataWriter.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestLabelPlacementMapper(int argc, char* argv[])
{
  // use non-unit aspect ratio to capture more potential errors
  int windowSize[2] = { 200, 600 };
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

  renWin->SetSize(200, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkTextProperty> tprop;
  tprop->SetFontSize(12);
  tprop->SetFontFamily(svtkTextProperty::GetFontFamilyFromString("Arial"));
  tprop->SetColor(0.0, 0.8, 0.2);

  // Test display if anchor is defined in
  // World coordinate system
  {
    int maxLevels = 5;
    int targetLabels = 32;
    double labelRatio = 0.05;
    char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uniform-001371-5x5x5.vtp");
    // int iteratorType = svtkLabelHierarchy::FULL_SORT;
    int iteratorType = svtkLabelHierarchy::QUEUE;
    // int iteratorType = svtkLabelHierarchy::DEPTH_FIRST;
    double center[3] = { 12.0, 8.0, 30.0 };

    svtkNew<svtkSphereSource> sphere;
    sphere->SetRadius(5.0);
    // view will be centered around this centerpoint, thereby shifting normalized view coordinate
    // system from world coordinate system (to test if label display works with acnhors defined in
    // arbitrary coordinate systems).
    sphere->SetCenter(center);
    svtkNew<svtkPolyDataMapper> sphereMapper;
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
    svtkNew<svtkActor> sphereActor;
    sphereActor->SetMapper(sphereMapper);
    renderer->AddActor(sphereActor);

    svtkNew<svtkXMLPolyDataReader> xmlPolyDataReader;
    xmlPolyDataReader->SetFileName(fname);
    delete[] fname;

    svtkNew<svtkTransformPolyDataFilter> transformToCenter;
    transformToCenter->SetInputConnection(xmlPolyDataReader->GetOutputPort());
    svtkNew<svtkTransform> transformToCenterTransform;
    transformToCenterTransform->Translate(center);
    transformToCenter->SetTransform(transformToCenterTransform);

    svtkNew<svtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputConnection(transformToCenter->GetOutputPort());
    pointSetToLabelHierarchy->SetPriorityArrayName("Priority");
    pointSetToLabelHierarchy->SetLabelArrayName("PlaceNames");
    pointSetToLabelHierarchy->SetMaximumDepth(maxLevels);
    pointSetToLabelHierarchy->SetTargetLabelCount(targetLabels);

    svtkNew<svtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->SetIteratorType(iteratorType);
    labelPlacer->SetMaximumLabelFraction(labelRatio);
    labelPlacer->UseDepthBufferOn();

    svtkNew<svtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  // Test display if anchor is defined in
  // NormalizedViewport coordinate system
  {
    svtkNew<svtkPolyData> labeledPoints;
    svtkNew<svtkPoints> points;
    points->InsertNextPoint(0.05, 0.25, 0);
    points->InsertNextPoint(0.75, 0.75, 0);
    points->InsertNextPoint(0.50, 0.05, 0);
    points->InsertNextPoint(0.50, 0.95, 0);
    labeledPoints->SetPoints(points);
    svtkNew<svtkStringArray> labels;
    labels->SetName("labels");
    labels->InsertNextValue("NV-left");
    labels->InsertNextValue("NV-right");
    labels->InsertNextValue("NV-bottom");
    labels->InsertNextValue("NV-top");
    svtkNew<svtkStringArray> labelsPriority;
    labelsPriority->SetName("priority");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labeledPoints->GetPointData()->AddArray(labels);
    labeledPoints->GetPointData()->AddArray(labelsPriority);
    svtkNew<svtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputData(labeledPoints);
    pointSetToLabelHierarchy->SetPriorityArrayName("priority");
    pointSetToLabelHierarchy->SetLabelArrayName("labels");
    svtkNew<svtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->PlaceAllLabelsOn();
    labelPlacer->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
    labelPlacer->UseDepthBufferOff();
    svtkNew<svtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  // Test display if anchor is defined in
  // Display coordinate system
  {
    svtkNew<svtkPolyData> labeledPoints;
    svtkNew<svtkPoints> points;
    points->InsertNextPoint(windowSize[0] * 0.01, windowSize[1] * 0.01, 0);
    points->InsertNextPoint(windowSize[0] * 0.90, windowSize[1] * 0.01, 0);
    points->InsertNextPoint(windowSize[0] * 0.01, windowSize[1] * 0.97, 0);
    points->InsertNextPoint(windowSize[0] * 0.90, windowSize[1] * 0.97, 0);
    labeledPoints->SetPoints(points);
    svtkNew<svtkStringArray> labels;
    labels->SetName("labels");
    labels->InsertNextValue("D-bottom-left");
    labels->InsertNextValue("D-bottom-right");
    labels->InsertNextValue("D-top-left");
    labels->InsertNextValue("D-top-right");
    svtkNew<svtkStringArray> labelsPriority;
    labelsPriority->SetName("priority");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labeledPoints->GetPointData()->AddArray(labels);
    labeledPoints->GetPointData()->AddArray(labelsPriority);
    svtkNew<svtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputData(labeledPoints);
    pointSetToLabelHierarchy->SetPriorityArrayName("priority");
    pointSetToLabelHierarchy->SetLabelArrayName("labels");
    svtkNew<svtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->PlaceAllLabelsOn();
    labelPlacer->GetAnchorTransform()->SetCoordinateSystemToDisplay();
    labelPlacer->UseDepthBufferOff();
    svtkNew<svtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  ///

  renWin->Render();
  // renderer->GetActiveCamera()->ParallelProjectionOn();
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
