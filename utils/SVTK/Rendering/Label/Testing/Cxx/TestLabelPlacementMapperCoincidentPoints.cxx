/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacementMapperCoincidentPoints.cxx

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
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkLabelHierarchy.h"
#include "svtkLabelPlacementMapper.h"
#include "svtkLabeledDataMapper.h"
#include "svtkMath.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
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
#include "svtkStringArray.h"
#include "svtkStructuredGrid.h"
#include "svtkTextProperty.h"
#include "svtkUnstructuredGrid.h"

#include "svtkSphereSource.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

/*
void prtbds( const char* msg, const double* bds )
{
  cout << msg << ":";
  for ( int i = 0; i < 3; ++ i )
    cout << " [ " << bds[2*i] << ", " << bds[2*i+1] << "]";
  cout << "\n";
}
*/

int TestLabelPlacementMapperCoincidentPoints(int argc, char* argv[])
{
  int maxLevels = 5;
  int targetLabels = 7;
  double labelRatio = 1.0;
  int i = 0;
  int iteratorType = svtkLabelHierarchy::QUEUE;

  svtkSmartPointer<svtkLabelHierarchy> labelHierarchy = svtkSmartPointer<svtkLabelHierarchy>::New();
  svtkSmartPointer<svtkLabelPlacementMapper> labelPlacer =
    svtkSmartPointer<svtkLabelPlacementMapper>::New();
  svtkSmartPointer<svtkPointSetToLabelHierarchy> pointSetToLabelHierarchy =
    svtkSmartPointer<svtkPointSetToLabelHierarchy>::New();

  svtkSmartPointer<svtkPolyDataMapper> polyDataMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkPolyDataMapper> polyDataMapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0); // ensure to have the same test image everywhere

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  svtkSmartPointer<svtkLabeledDataMapper> labeledMapper =
    svtkSmartPointer<svtkLabeledDataMapper>::New();
  svtkSmartPointer<svtkActor2D> textActor = svtkSmartPointer<svtkActor2D>::New();

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  // renderer->GetActiveCamera()->ParallelProjectionOn();

  svtkMath::RandomSeed(5678);

  for (i = 0; i < 29; i++)
  {
    // points->InsertPoint( i, svtkMath::Random(-1.0, 1.0), svtkMath::Random(-1.0, 1.0), 0. );
    points->InsertPoint(i, 0.0, 0.0, 0.0);
  }
  points->InsertPoint(29, 2.2, 2.2, 0.0);

  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();

  cells->InsertNextCell(30);
  for (i = 0; i < 30; i++)
  {
    cells->InsertCellPoint(i);
  }

  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetVerts(cells);

  svtkSmartPointer<svtkStringArray> stringData = svtkSmartPointer<svtkStringArray>::New();
  stringData->SetName("PlaceNames");
  stringData->InsertNextValue("Abu Dhabi");
  stringData->InsertNextValue("Amsterdam");
  stringData->InsertNextValue("Beijing");
  stringData->InsertNextValue("Berlin");
  stringData->InsertNextValue("Cairo");
  stringData->InsertNextValue("Caracas");
  stringData->InsertNextValue("Dublin");
  stringData->InsertNextValue("Georgetown");
  stringData->InsertNextValue("The Hague");
  stringData->InsertNextValue("Hanoi");
  stringData->InsertNextValue("Islamabad");
  stringData->InsertNextValue("Jakarta");
  stringData->InsertNextValue("Kiev");
  stringData->InsertNextValue("Kingston");
  stringData->InsertNextValue("Lima");
  stringData->InsertNextValue("London");
  stringData->InsertNextValue("Luxembourg City");
  stringData->InsertNextValue("Madrid");
  stringData->InsertNextValue("Moscow");
  stringData->InsertNextValue("Nairobi");
  stringData->InsertNextValue("New Delhi");
  stringData->InsertNextValue("Ottawa");
  stringData->InsertNextValue("Paris");
  stringData->InsertNextValue("Prague");
  stringData->InsertNextValue("Rome");
  stringData->InsertNextValue("Seoul");
  stringData->InsertNextValue("Tehran");
  stringData->InsertNextValue("Tokyo");
  stringData->InsertNextValue("Warsaw");
  stringData->InsertNextValue("Washington");

  polyData->GetPointData()->AddArray(stringData);

  svtkSmartPointer<svtkTextProperty> tprop = svtkSmartPointer<svtkTextProperty>::New();
  tprop->SetFontSize(12);
  tprop->SetFontFamily(svtkTextProperty::GetFontFamilyFromString("Arial"));
  tprop->SetColor(0.0, 0.8, 0.2);

  pointSetToLabelHierarchy->SetInputData(polyData);
  pointSetToLabelHierarchy->SetTextProperty(tprop);
  pointSetToLabelHierarchy->SetPriorityArrayName("Priority");
  pointSetToLabelHierarchy->SetLabelArrayName("PlaceNames");
  pointSetToLabelHierarchy->SetMaximumDepth(maxLevels);
  pointSetToLabelHierarchy->SetTargetLabelCount(targetLabels);

  labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
  labelPlacer->SetIteratorType(iteratorType);
  labelPlacer->SetMaximumLabelFraction(labelRatio);
  // labelPlacer->SetIteratorType(1); // Quadtree is only available type for 2-D.

  polyDataMapper->SetInputData(polyData);
  // polyDataMapper2->SetInputConnection(labelPlacer->GetOutputPort(2));

  actor->SetMapper(polyDataMapper);
  // actor2->SetMapper(polyDataMapper2);

  // labelPlacer->Update();

  textActor->SetMapper(labelPlacer);

  // renderer->AddActor(actor);
  // renderer->AddActor(actor2);
  renderer->AddActor(textActor);

  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  iren->SetRenderWindow(renWin);

  // labelPlacer->Update();
  // cout << "Pre-reset-camera bounds of...\n";
  // prtbds( "output 0", labelPlacer->GetOutput( 0 )->GetBounds() );
  // prtbds( "output 1", labelPlacer->GetOutput( 1 )->GetBounds() );
  // prtbds( "output 2", labelPlacer->GetOutput( 2 )->GetBounds() );
  /*
  renWin->Render();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renWin->Render();
  */
  // cout << "Post-reset-camera Bounds of...\n";
  // prtbds( "output 0", labelPlacer->GetOutput( 0 )->GetBounds() );
  // prtbds( "output 1", labelPlacer->GetOutput( 1 )->GetBounds() );
  // prtbds( "output 2", labelPlacer->GetOutput( 2 )->GetBounds() );
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
