/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadRotationalExtrusion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2012

#include "svtkSmartPointer.h"

#include "svtkAVSucdReader.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataSetMapper.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"
#include "svtkYoungsMaterialInterface.h"

//----------------------------------------------------------------------------
int TestYoungsMaterialInterface(int argc, char* argv[])
{
  // Create renderer and add actors to it
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(.8, .8, .8);

  // Create render window;
  svtkNew<svtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->SetSize(500, 200);
  window->SetMultiSamples(0);

  // Create interactor;
  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  // Read from AVS UCD data in binary form;
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/UCD2D/UCD_00005.inp");
  svtkNew<svtkAVSucdReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  // Update reader and get mesh cell data;
  reader->Update();
  svtkUnstructuredGrid* mesh = reader->GetOutput();
  svtkCellData* cellData = mesh->GetCellData();

  // Create normal vectors;
  cellData->SetActiveScalars("norme[0]");
  svtkDataArray* normX = cellData->GetScalars();
  cellData->SetActiveScalars("norme[1]");
  svtkDataArray* normY = cellData->GetScalars();
  svtkIdType n = normX->GetNumberOfTuples();
  svtkNew<svtkDoubleArray> norm;
  norm->SetNumberOfComponents(3);
  norm->SetNumberOfTuples(n);
  norm->SetName("norme");
  for (int i = 0; i < n; ++i)
  {
    norm->SetTuple3(i, normX->GetTuple1(i), normY->GetTuple1(i), 0.);
  }
  cellData->SetVectors(norm);

  // Extract submesh corresponding with cells containing material 2
  cellData->SetActiveScalars("Material Id");
  svtkNew<svtkThreshold> threshold2;
  threshold2->SetInputData(mesh);
  threshold2->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
  threshold2->ThresholdByLower(2);
  threshold2->Update();
  svtkUnstructuredGrid* meshMat2 = threshold2->GetOutput();

  // Extract submesh corresponding with cells containing material 3
  svtkNew<svtkThreshold> threshold3;
  threshold3->SetInputData(mesh);
  threshold3->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
  threshold3->ThresholdByUpper(3);
  threshold3->Update();
  svtkUnstructuredGrid* meshMat3 = threshold3->GetOutput();

  // Make multiblock from extracted submeshes;
  svtkNew<svtkMultiBlockDataSet> meshMB;
  meshMB->SetNumberOfBlocks(2);
  meshMB->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Material 2");
  meshMB->SetBlock(0, meshMat2);
  meshMB->GetMetaData(static_cast<unsigned>(1))->Set(svtkCompositeDataSet::NAME(), "Material 3");
  meshMB->SetBlock(1, meshMat3);

  // Create mapper for submesh corresponding to material 2;
  double* matRange = cellData->GetScalars()->GetRange();
  svtkNew<svtkDataSetMapper> meshMapper;
  meshMapper->SetInputData(meshMat2);
  meshMapper->SetScalarRange(matRange);
  meshMapper->SetScalarModeToUseCellData();
  meshMapper->SetColorModeToMapScalars();
  meshMapper->ScalarVisibilityOn();
  meshMapper->SetResolveCoincidentTopologyPolygonOffsetParameters(0, 1);
  meshMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create wireframe actor for entire mesh
  svtkNew<svtkActor> meshActor;
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddViewProp(meshActor);

  cellData->SetActiveScalars("frac_pres[1]");
  // Reconstruct Youngs material interface
  svtkNew<svtkYoungsMaterialInterface> youngs;
  youngs->SetInputData(meshMB);
  youngs->SetNumberOfMaterials(2);
  youngs->SetMaterialVolumeFractionArray(0, "frac_pres[1]");
  youngs->SetMaterialVolumeFractionArray(1, "frac_pres[2]");
  youngs->SetMaterialNormalArray(0, "norme");
  youngs->SetMaterialNormalArray(1, "norme");
  youngs->SetVolumeFractionRange(.001, .999);
  youngs->FillMaterialOn();
  youngs->RemoveAllMaterialBlockMappings();
  youngs->AddMaterialBlockMapping(-1);
  youngs->AddMaterialBlockMapping(1);
  youngs->AddMaterialBlockMapping(-2);
  youngs->AddMaterialBlockMapping(2);
  youngs->UseAllBlocksOff();
  youngs->Update();

  // Create mappers and actors for surface rendering of all reconstructed interfaces;
  svtkSmartPointer<svtkCompositeDataIterator> interfaceIterator;
  interfaceIterator.TakeReference(youngs->GetOutput()->NewIterator());
  interfaceIterator->SkipEmptyNodesOn();
  interfaceIterator->InitTraversal();
  interfaceIterator->GoToFirstItem();
  while (interfaceIterator->IsDoneWithTraversal() == 0)
  {
    // Select blue component of leaf mesh
    double bComp = interfaceIterator->GetCurrentFlatIndex() == 2 ? 0 : 1;

    // Fetch interface object and downcast to data set
    svtkDataObject* interfaceDO = interfaceIterator->GetCurrentDataObject();
    svtkDataSet* interface = svtkDataSet::SafeDownCast(interfaceDO);

    // Create mapper for interface
    svtkNew<svtkDataSetMapper> interfaceMapper;
    interfaceMapper->SetInputData(interface);
    interfaceIterator->GoToNextItem();
    interfaceMapper->ScalarVisibilityOff();
    interfaceMapper->SetResolveCoincidentTopologyPolygonOffsetParameters(1, 100);
    interfaceMapper->SetResolveCoincidentTopologyToPolygonOffset();

    // Create surface actor and add it to view
    svtkNew<svtkActor> interfaceActor;
    interfaceActor->SetMapper(interfaceMapper);
    interfaceActor->GetProperty()->SetColor(0., 1 - bComp, bComp);
    interfaceActor->GetProperty()->SetRepresentationToSurface();
    renderer->AddViewProp(interfaceActor);
  }

  // Render and test;
  window->Render();
  interactor->Start();

  return EXIT_SUCCESS;
}
