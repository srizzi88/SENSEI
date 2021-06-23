/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestViewDependentErrorMetric.cxx

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
#include "svtkActor2D.h"
#include "svtkAttributesErrorMetric.h"
#include "svtkBridgeDataSet.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataSetMapper.h"
#include "svtkDebugLeaks.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSetTessellator.h"
#include "svtkGenericGeometryFilter.h"
#include "svtkGenericOutlineFilter.h"
#include "svtkGenericSubdivisionErrorMetric.h"
#include "svtkGeometricErrorMetric.h"
#include "svtkLabeledDataMapper.h"
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
#include "svtkViewDependentErrorMetric.h"
#include "svtkXMLUnstructuredGridReader.h"
#include <cassert>

#ifdef WRITE_GENERIC_RESULT
#include "svtkXMLUnstructuredGridWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

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

  virtual void Execute(svtkObject* svtkNotUsed(caller), unsigned long, void*)
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

int TestViewDependentErrorMetric(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderer* renderer2 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renWin->AddRenderer(renderer2);
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
#if 0
  // Set the error metric thresholds:
  // 1. for the geometric error metric
  svtkGeometricErrorMetric *geometricError=svtkGeometricErrorMetric::New();
  geometricError->SetRelativeGeometricTolerance(0.01,ds);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(geometricError);
  geometricError->Delete();

  // 2. for the attribute error metric
  svtkAttributesErrorMetric *attributesError=svtkAttributesErrorMetric::New();
  attributesError->SetAttributeTolerance(0.01); // 0.11

  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);
  attributesError->Delete();
#endif

  // 3. for the view dependent error metric on the first renderer
  svtkViewDependentErrorMetric* viewError = svtkViewDependentErrorMetric::New();
  viewError->SetViewport(renderer);
  viewError->SetPixelTolerance(10000); // 0.25; 0.0625
  ds->GetTessellator()->GetErrorMetrics()->AddItem(viewError);
  viewError->Delete();

  // 4. for the view dependent error metric on the first renderer
  svtkViewDependentErrorMetric* viewError2 = svtkViewDependentErrorMetric::New();
  viewError2->SetViewport(renderer2);
  viewError2->SetPixelTolerance(0.25); // 0.25; 0.0625
  ds->GetTessellator()->GetErrorMetrics()->AddItem(viewError2);
  viewError2->Delete();

  cout << "input unstructured grid: " << ds << endl;

  static_cast<svtkSimpleCellTessellator*>(ds->GetTessellator())->SetMaxSubdivisionLevel(10);

  svtkIndent indent;
  ds->PrintSelf(cout, indent);

#if 0
  // Create the filter
  svtkGenericDataSetTessellator *tessellator = svtkGenericDataSetTessellator::New();
  tessellator->SetInputData(ds);

  // DO NOT PERFORM UPDATE NOW, because the view dependent error metric
  // need the window to be realized first
  //tessellator->Update(); //So that we can call GetRange() on the scalars

  assert(tessellator->GetOutput()!=0);
#else
  // Create the filter
  svtkGenericGeometryFilter* tessellator = svtkGenericGeometryFilter::New();
  tessellator->SetInputData(ds);

  //  geom->Update(); //So that we can call GetRange() on the scalars

#endif

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);

  svtkDataSetMapper* mapper = svtkDataSetMapper::New();
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection(tessellator->GetOutputPort());

  int i = 0;
  int n = ds->GetAttributes()->GetNumberOfAttributes();
  int found = 0;
  svtkGenericAttribute* attribute = 0;
  while (i < n && !found)
  {
    attribute = ds->GetAttributes()->GetAttribute(i);
    found =
      (attribute->GetCentering() == svtkPointCentered && attribute->GetNumberOfComponents() == 1);
    ++i;
  }
  if (found)
  {
    mapper->SetScalarRange(attribute->GetRange(0));
  }
  mapper->ScalarVisibilityOff();

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

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
  renderer->SetBackground(0.7, 0.5, 0.5);
  renderer->SetViewport(0, 0, 0.5, 1);
  renderer2->SetBackground(0.5, 0.5, 0.8);
  renderer2->SetViewport(0.5, 0, 1, 1);
  renWin->SetSize(600, 300); // realized

  svtkGenericOutlineFilter* outlineFilter = svtkGenericOutlineFilter::New();
  outlineFilter->SetInputData(ds);
  svtkPolyDataMapper* mapperOutline = svtkPolyDataMapper::New();
  mapperOutline->SetInputConnection(outlineFilter->GetOutputPort());
  outlineFilter->Delete();

  svtkActor* actorOutline = svtkActor::New();
  actorOutline->SetMapper(mapperOutline);
  mapperOutline->Delete();

  renderer->AddActor(actorOutline);
  renderer2->AddActor(actorOutline);
  actorOutline->Delete();
  // need an outline filter in the pipeline to ensure that the
  // camera are set with the bounding box of the dataset.

  //  svtkCamera *cam1=renderer->GetActiveCamera();
  svtkCamera* cam2 = renderer2->GetActiveCamera();

  renderer->ResetCamera();
  renderer2->ResetCamera();

  cam2->Azimuth(90);

  // Those two lines have to be called AFTER GetActiveCamera:
  // GetActiveCamera ask the mapper to update its input for the bounds
  // If the actor is connected it actually ask the output of tessellator
  // but the view dependent error metric are not yet initialized!
  renderer->AddActor(actor);
  renderer2->AddActor(actor);

  renWin->Render();

#ifdef WRITE_GENERIC_RESULT
  // BE SURE to save AFTER a first rendering!
  // Save the result of the filter in a file
  svtkXMLUnstructuredGridWriter* writer = svtkXMLUnstructuredGridWriter::New();
  writer->SetInputConnection(tessellator->GetOutputPort());
  writer->SetFileName("viewdeptessellated.vtu");
  writer->SetDataModeToAscii();
  writer->DebugOn();
  writer->Write();
  writer->Delete();

  // debug XML reader
  svtkXMLUnstructuredGridReader* rreader = svtkXMLUnstructuredGridReader::New();
  //  rreader->SetInputConnection(tessellator->GetOutputPort());
  rreader->SetFileName("viewdeptessellated.vtu");
  //  rreader->SetDataModeToAscii();
  rreader->DebugOn();
  rreader->Update();
  rreader->Delete();

#endif // #ifdef WRITE_GENERIC_RESULT

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
  renderer2->Delete();
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
