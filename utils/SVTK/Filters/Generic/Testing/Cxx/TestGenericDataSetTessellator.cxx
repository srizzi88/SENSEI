/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericDataSetTessellator.cxx

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

#define WRITE_GENERIC_RESULT

#define WITH_GEOMETRY_FILTER

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkCommand.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSetTessellator.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkLabeledDataMapper.h"
#include "svtkLookupTable.h"
#include "svtkPointData.h"
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

#ifdef WITH_GEOMETRY_FILTER
#include "svtkGeometryFilter.h"
#include "svtkPolyDataMapper.h"
#endif

#ifdef WRITE_GENERIC_RESULT
#include "svtkXMLUnstructuredGridWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

// debugging when clipping on n=(1,1,1) c=(0.5,0,0)
//#include "svtkExtractGeometry.h"
//#include "svtkSphere.h"

// Remark about the lookup tables that seem different between the
// GenericGeometryFilter and GenericDataSetTessellator:
// the lookup table is set for the whole unstructured grid, the tetra plus
// the triangle. The lookup table changed because of the tetra: the
// GenericDataSetTessellator need to create inside sub-tetra that have
// minimal attributes, the GenericGeometryFilter just need to tessellate the
// face of the tetra, for which the values at points are not minimal.

class SwitchLabelsCallback : public svtkCommand
{
public:
  static SwitchLabelsCallback* New() { return new SwitchLabelsCallback; }

  void SetLabeledDataMapper(svtkLabeledDataMapper* aLabeledDataMapper)
  {
    this->LabeledDataMapper = aLabeledDataMapper;
  }
  void SetRenderWindow(svtkRenderWindow* aRenWin) { this->RenWin = aRenWin; }

  void Execute(svtkObject* svtkNotUsed(caller), unsigned long, void*) override
  {
    if (this->LabeledDataMapper->GetLabelMode() == SVTK_LABEL_SCALARS)
    {
      this->LabeledDataMapper->SetLabelMode(SVTK_LABEL_IDS);
    }
    else
    {
      this->LabeledDataMapper->SetLabelMode(SVTK_LABEL_SCALARS);
    }
    this->RenWin->Render();
  }

protected:
  svtkLabeledDataMapper* LabeledDataMapper;
  svtkRenderWindow* RenWin;
};

int TestGenericDataSetTessellator(int argc, char* argv[])
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
  attributesError->SetAttributeTolerance(0.01); // 0.11, 0.005

  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);
  attributesError->Delete();
  cout << "input unstructured grid: " << ds << endl;

  static_cast<svtkSimpleCellTessellator*>(ds->GetTessellator())->SetSubdivisionLevels(0, 100);
  svtkIndent indent;
  ds->PrintSelf(cout, indent);

  // Create the filter
  svtkGenericDataSetTessellator* tessellator = svtkGenericDataSetTessellator::New();
  tessellator->SetInputData(ds);

  tessellator->Update(); // So that we can call GetRange() on the scalars

  assert(tessellator->GetOutput() != nullptr);

  // for debugging clipping on the hexa
#if 0
  svtkExtractGeometry *eg=svtkExtractGeometry::New();
  eg->SetInputConnection(tessellator->GetOutputPort());

  svtkSphere *sphere=svtkSphere::New();
  sphere->SetRadius(0.1);
  sphere->SetCenter(0,0,0);

  eg->SetImplicitFunction(sphere);
  eg->SetExtractInside(1);
  eg->SetExtractBoundaryCells(0);

  svtkXMLUnstructuredGridWriter *cwriter=svtkXMLUnstructuredGridWriter::New();
  cwriter->SetInputConnection(eg->GetOutputPort());
  cwriter->SetFileName("extracted_tessellated.vtu");
  cwriter->SetDataModeToAscii();
  cwriter->Write();

  cwriter->Delete();
  sphere->Delete();
  eg->Delete();
#endif

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);

#ifdef WITH_GEOMETRY_FILTER
  svtkGeometryFilter* geom = svtkGeometryFilter::New();
  geom->SetInputConnection(tessellator->GetOutputPort());
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(geom->GetOutputPort());
  geom->Delete();
#else
  svtkDataSetMapper* mapper = svtkDataSetMapper::New();
  mapper->SetInputConnection(tessellator->GetOutputPort());
#endif
  mapper->SetLookupTable(lut);
  if (tessellator->GetOutput()->GetPointData() != nullptr)
  {
    if (tessellator->GetOutput()->GetPointData()->GetScalars() != nullptr)
    {
      mapper->SetScalarRange(tessellator->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

#ifdef WRITE_GENERIC_RESULT
  // Save the result of the filter in a file
  svtkXMLUnstructuredGridWriter* writer = svtkXMLUnstructuredGridWriter::New();
  writer->SetInputConnection(tessellator->GetOutputPort());
  writer->SetFileName("tessellated.vtu");
  writer->SetDataModeToAscii();
  writer->Write();
  writer->Delete();
#endif // #ifdef WRITE_GENERIC_RESULT

  svtkActor2D* actorLabel = svtkActor2D::New();
  svtkLabeledDataMapper* labeledDataMapper = svtkLabeledDataMapper::New();
  labeledDataMapper->SetLabelMode(SVTK_LABEL_IDS);
  labeledDataMapper->SetInputConnection(tessellator->GetOutputPort());
  actorLabel->SetMapper(labeledDataMapper);
  labeledDataMapper->Delete();
  renderer->AddActor(actorLabel);
  actorLabel->SetVisibility(0);
  actorLabel->Delete();

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();

  tessellator->GetOutput()->PrintSelf(cout, indent);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    SwitchLabelsCallback* switchLabels = SwitchLabelsCallback::New();
    switchLabels->SetRenderWindow(renWin);
    switchLabels->SetLabeledDataMapper(labeledDataMapper);
    iren->AddObserver(svtkCommand::UserEvent, switchLabels);
    switchLabels->Delete();
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  actor->Delete();
  tessellator->Delete();
  ds->Delete();
  lut->Delete();

  return !retVal;
}
