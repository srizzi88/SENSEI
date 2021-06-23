/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericGlyph3DFilter.cxx

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
#include "svtkArrowSource.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericGeometryFilter.h"
#include "svtkGenericGlyph3DFilter.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkLookupTable.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
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

int TestGenericGlyph3DFilter(int argc, char* argv[])
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
  svtkArrowSource* arrow = svtkArrowSource::New();
  svtkGenericGlyph3DFilter* glyph = svtkGenericGlyph3DFilter::New();
  glyph->SetInputData(ds);
  glyph->SetInputConnection(1, arrow->GetOutputPort());
  glyph->SetScaling(1);
  glyph->SetScaleModeToScaleByScalar();
  glyph->SelectInputScalars("scalars");
  glyph->SetColorModeToColorByScale();

  svtkPolyDataMapper* glyphMapper = svtkPolyDataMapper::New();
  glyphMapper->SetInputConnection(glyph->GetOutputPort());
  svtkActor* glyphActor = svtkActor::New();
  glyphActor->SetMapper(glyphMapper);
  renderer->AddActor(glyphActor);

  // Create the filter
  svtkGenericGeometryFilter* geom = svtkGenericGeometryFilter::New();
  geom->SetInputData(ds);

  geom->Update(); // So that we can call GetRange() on the scalars

  assert(geom->GetOutput() != nullptr);

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection(geom->GetOutputPort());

  if (geom->GetOutput()->GetPointData() != nullptr)
  {
    if (geom->GetOutput()->GetPointData()->GetScalars() != nullptr)
    {
      mapper->SetScalarRange(geom->GetOutput()->GetPointData()->GetScalars()->GetRange());
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
  geom->Delete();
  ds->Delete();
  lut->Delete();
  glyphActor->Delete();
  glyphMapper->Delete();
  glyph->Delete();
  arrow->Delete();

  return !retVal;
}
