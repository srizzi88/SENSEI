/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to implement a svtkGenericDataSet
// (here svtkBridgeDataSet) and to use svtkGenericDataSetTessellator filter on
// it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "svtkActor.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericCutter.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkLookupTable.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimpleCellTessellator.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"
#include <cassert>

int TestGenericCutter(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  svtkXMLUnstructuredGridReader* reader = svtkXMLUnstructuredGridReader::New();
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");
  reader->SetFileName(cfname);
  delete[] cfname;

  // Force reading
  reader->Update();

  // Initialize the bridge
  svtkBridgeDataSet* ds = svtkBridgeDataSet::New();
  ds->SetDataSet(reader->GetOutput());
  reader->Delete();

  // Set the error metric thresholds:
  // 1. for the geometric error metric
  svtkGeometricErrorMetric* geometricError = svtkGeometricErrorMetric::New();
  geometricError->SetRelativeGeometricTolerance(0.1, ds);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(geometricError);
  geometricError->Delete();

  // 2. for the attribute error metric
  svtkAttributesErrorMetric* attributesError = svtkAttributesErrorMetric::New();
  attributesError->SetAttributeTolerance(0.01);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);
  attributesError->Delete();
  cout << "input unstructured grid: " << ds << endl;

  static_cast<svtkSimpleCellTessellator*>(ds->GetTessellator())->SetMaxSubdivisionLevel(10);

  svtkIndent indent;
  ds->PrintSelf(cout, indent);

  // Create the filter

  svtkPlane* implicitPlane = svtkPlane::New();
  implicitPlane->SetOrigin(0.5, 0, 0); // 0, -1, 0
  implicitPlane->SetNormal(1, 1, 1);

  svtkGenericCutter* cutter = svtkGenericCutter::New();
  cutter->SetInputData(ds);

  cutter->SetCutFunction(implicitPlane);
  implicitPlane->Delete();
  cutter->SetValue(0, 0.5);
  cutter->GenerateCutScalarsOn();

  cutter->Update(); // So that we can call GetRange() on the scalars

  assert(cutter->GetOutput() != nullptr);

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);

  svtkDataSetMapper* mapper = svtkDataSetMapper::New();
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection(cutter->GetOutputPort());

  if (cutter->GetOutput()->GetPointData() != nullptr)
  {
    if (cutter->GetOutput()->GetPointData()->GetScalars() != nullptr)
    {
      mapper->SetScalarRange(cutter->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  actor->Delete();
  cutter->Delete();
  ds->Delete();
  lut->Delete();

  return !retVal;
}
