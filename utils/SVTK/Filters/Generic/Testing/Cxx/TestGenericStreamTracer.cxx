/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericStreamTracer.cxx

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

//#define WRITE_GENERIC_RESULT

#include "svtkActor.h"
#include "svtkAssignAttribute.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkCamera.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericOutlineFilter.h"
#include "svtkGenericStreamTracer.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRibbonFilter.h"
#include "svtkRungeKutta45.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridReader.h"
#include "svtkTestUtilities.h"
#include <cassert>

#ifdef WRITE_GENERIC_RESULT
#include "svtkXMLPolyDataWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

int TestGenericStreamTracer(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  svtkStructuredGridReader* reader = svtkStructuredGridReader::New();
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/office.binary.svtk");
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

  svtkIndent indent;
  ds->PrintSelf(cout, indent);

  svtkGenericOutlineFilter* outline = svtkGenericOutlineFilter::New();
  outline->SetInputData(ds);
  svtkPolyDataMapper* mapOutline = svtkPolyDataMapper::New();
  mapOutline->SetInputConnection(outline->GetOutputPort());
  svtkActor* outlineActor = svtkActor::New();
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(0, 0, 0);

  svtkRungeKutta45* rk = svtkRungeKutta45::New();

  // Create source for streamtubes
  svtkGenericStreamTracer* streamer = svtkGenericStreamTracer::New();
  streamer->SetInputData(ds);
  streamer->SetStartPosition(0.1, 2.1, 0.5);
  streamer->SetMaximumPropagation(0, 500);
  streamer->SetMinimumIntegrationStep(1, 0.1);
  streamer->SetMaximumIntegrationStep(1, 1.0);
  streamer->SetInitialIntegrationStep(2, 0.2);
  streamer->SetIntegrationDirection(0);
  streamer->SetIntegrator(rk);
  streamer->SetRotationScale(0.5);
  streamer->SetMaximumError(1.0E-8);

  svtkAssignAttribute* aa = svtkAssignAttribute::New();
  aa->SetInputConnection(streamer->GetOutputPort());
  aa->Assign("Normals", svtkDataSetAttributes::NORMALS, svtkAssignAttribute::POINT_DATA);

  svtkRibbonFilter* rf1 = svtkRibbonFilter::New();
  rf1->SetInputConnection(aa->GetOutputPort());
  rf1->SetWidth(0.1);
  rf1->VaryWidthOff();

  svtkPolyDataMapper* mapStream = svtkPolyDataMapper::New();
  mapStream->SetInputConnection(rf1->GetOutputPort());
  mapStream->SetScalarRange(ds->GetAttributes()->GetAttribute(0)->GetRange());
  svtkActor* streamActor = svtkActor::New();
  streamActor->SetMapper(mapStream);

  renderer->AddActor(outlineActor);
  renderer->AddActor(streamActor);

  svtkCamera* cam = renderer->GetActiveCamera();
  cam->SetPosition(-2.35599, -3.35001, 4.59236);
  cam->SetFocalPoint(2.255, 2.255, 1.28413);
  cam->SetViewUp(0.311311, 0.279912, 0.908149);
  cam->SetClippingRange(1.12294, 16.6226);

#ifdef WRITE_GENERIC_RESULT
  // Save the result of the filter in a file
  svtkXMLPolyDataWriter* writer = svtkXMLPolyDataWriter::New();
  writer->SetInputConnection(streamer->GetOutputPort());
  writer->SetFileName("streamed.vtu");
  writer->SetDataModeToAscii();
  writer->Write();
  writer->Delete();
#endif // #ifdef WRITE_GENERIC_RESULT

  // Standard testing code.
  renderer->SetBackground(0.4, 0.4, 0.5);
  renWin->SetSize(300, 200);
  renWin->Render();
  streamer->GetOutput()->PrintSelf(cout, indent);
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  ds->Delete();

  outline->Delete();
  mapOutline->Delete();
  outlineActor->Delete();

  rk->Delete();
  streamer->Delete();
  aa->Delete();
  rf1->Delete();
  mapStream->Delete();
  streamActor->Delete();

  return !retVal;
}
