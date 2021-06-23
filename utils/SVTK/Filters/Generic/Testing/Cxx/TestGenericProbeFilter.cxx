/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to implement a svtkGenericDataSet
// (here svtkBridgeDataSet) and to use svtkGenericProbeFilter on it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

//#define ADD_GEOMETRY
//#define STD_PROBE

#include "svtkActor.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericGeometryFilter.h"
#include "svtkGenericProbeFilter.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkLookupTable.h"
#include "svtkPlane.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProbeFilter.h" // std probe filter, to compare
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimpleCellTessellator.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"
#include <cassert>

int TestGenericProbeFilter(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

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

#ifdef ADD_GEOMETRY
  // Geometry

  // Create the filter
  svtkGenericGeometryFilter* geom = svtkGenericGeometryFilter::New();
  geom->SetInputData(ds);

  geom->Update(); // So that we can call GetRange() on the scalars

  assert(geom->GetOutput() != 0);

  // This creates a blue to red lut.
  svtkLookupTable* lut2 = svtkLookupTable::New();
  lut2->SetHueRange(0.667, 0.0);

  svtkPolyDataMapper* mapper2 = svtkPolyDataMapper::New();
  mapper2->SetLookupTable(lut2);
  lut2->Delete();
  mapper2->SetInputConnection(0, geom->GetOutputPort(0));
  geom->Delete();

  if (geom->GetOutput()->GetPointData() != 0)
  {
    if (geom->GetOutput()->GetPointData()->GetScalars() != 0)
    {
      mapper2->SetScalarRange(geom->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }
  svtkActor* actor2 = svtkActor::New();
  actor2->SetMapper(mapper2);
  mapper2->Delete();
  renderer->AddActor(actor2); // the surface
  actor2->Delete();
#endif

  // Create the probe plane
  svtkPlaneSource* plane = svtkPlaneSource::New();
  plane->SetResolution(100, 100);
  svtkTransform* transp = svtkTransform::New();
  transp->Translate(0.5, 0.5, 0);
  transp->Scale(5, 5, 5);
  svtkTransformPolyDataFilter* tpd = svtkTransformPolyDataFilter::New();
  tpd->SetInputConnection(0, plane->GetOutputPort(0));
  plane->Delete();
  tpd->SetTransform(transp);
  transp->Delete();

#ifndef STD_PROBE
  // Create the filter
  svtkGenericProbeFilter* probe = svtkGenericProbeFilter::New();
  probe->SetInputConnection(0, tpd->GetOutputPort(0));
  tpd->Delete();
  probe->SetSourceData(ds);

  probe->Update(); // So that we can call GetRange() on the scalars

  assert(probe->GetOutput() != nullptr);

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);

  svtkDataSetMapper* mapper = svtkDataSetMapper::New();
  mapper->SetLookupTable(lut);
  lut->Delete();
  mapper->SetInputConnection(0, probe->GetOutputPort(0));
  probe->Delete();

  if (probe->GetOutput()->GetPointData() != nullptr)
  {
    if (probe->GetOutput()->GetPointData()->GetScalars() != nullptr)
    {
      mapper->SetScalarRange(probe->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();
  renderer->AddActor(actor);
  actor->Delete();

#else
  // std probe, to compare

  svtkProbeFilter* stdProbe = svtkProbeFilter::New();
  stdProbe->SetInputConnection(0, tpd->GetOutputPort(0));
  tpd->Delete();
  stdProbe->SetSourceData(ds->GetDataSet());

  stdProbe->Update(); // So that we can call GetRange() on the scalars

  assert(stdProbe->GetOutput() != 0);

  // This creates a blue to red lut.
  svtkLookupTable* lut4 = svtkLookupTable::New();
  lut4->SetHueRange(0.667, 0.0);

  svtkDataSetMapper* mapper4 = svtkDataSetMapper::New();
  mapper4->SetLookupTable(lut4);
  lut4->Delete();
  mapper4->SetInputConnection(0, stdProbe->GetOutputPort(0));
  stdProbe->Delete();

  if (stdProbe->GetOutput()->GetPointData() != 0)
  {
    if (stdProbe->GetOutput()->GetPointData()->GetScalars() != 0)
    {
      mapper4->SetScalarRange(stdProbe->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }

  svtkActor* actor4 = svtkActor::New();
  actor4->SetMapper(mapper4);
  mapper4->Delete();
  renderer->AddActor(actor4);
  actor4->Delete();
#endif // #ifdef STD_PROBE

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
  iren->Delete();
  ds->Delete();

  return !retVal;
}
