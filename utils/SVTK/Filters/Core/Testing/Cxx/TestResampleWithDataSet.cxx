/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkResampleWithDataSet.h"

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkCylinder.h"
#include "svtkExtentTranslator.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

namespace
{

void CreateInputDataSet(svtkMultiBlockDataSet* dataset, int numberOfBlocks)
{
  dataset->SetNumberOfBlocks(numberOfBlocks);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(15);
  cylinder->SetAxis(0, 1, 0);
  svtkNew<svtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetClipFunction(cylinder);
  clipCyl->InsideOutOn();

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(12);
  svtkNew<svtkTableBasedClipDataSet> clipSphr;
  clipSphr->SetInputConnection(clipCyl->GetOutputPort());
  clipSphr->SetClipFunction(sphere);

  svtkNew<svtkTransform> transform;
  transform->RotateZ(45);
  svtkNew<svtkTransformFilter> transFilter;
  transFilter->SetInputConnection(clipSphr->GetOutputPort());
  transFilter->SetTransform(transform);

  svtkNew<svtkRandomAttributeGenerator> randomAttrs;
  randomAttrs->SetInputConnection(transFilter->GetOutputPort());
  randomAttrs->GenerateAllPointDataOn();
  randomAttrs->GeneratePointArrayOff();
  randomAttrs->GenerateAllCellDataOn();
  randomAttrs->GenerateCellArrayOff();
  randomAttrs->GenerateFieldArrayOn();
  randomAttrs->SetNumberOfTuples(100);

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    clipCyl->SetInputData(wavelet->GetOutputDataObject(0));
    randomAttrs->Update();

    svtkDataObject* block = randomAttrs->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(randomAttrs->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

void CreateSourceDataSet(svtkMultiBlockDataSet* dataset, int numberOfBlocks)
{
  dataset->SetNumberOfBlocks(numberOfBlocks);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);

    svtkDataObject* block = wavelet->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(wavelet->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

} // anonymous namespace

int TestResampleWithDataSet(int argc, char* argv[])
{
  // create input dataset
  svtkNew<svtkMultiBlockDataSet> input;
  CreateInputDataSet(input, 3);

  svtkNew<svtkMultiBlockDataSet> source;
  CreateSourceDataSet(source, 5);

  svtkNew<svtkResampleWithDataSet> resample;
  resample->SetInputData(input);
  resample->SetSourceData(source);

  // test default output
  resample->Update();
  svtkMultiBlockDataSet* result = static_cast<svtkMultiBlockDataSet*>(resample->GetOutput());
  svtkDataSet* block = static_cast<svtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 1 ||
    block->GetCellData()->GetNumberOfArrays() != 1 ||
    block->GetPointData()->GetNumberOfArrays() != 3)
  {
    std::cout << "Unexpected number of arrays in default output" << std::endl;
    return !svtkTesting::FAILED;
  }

  // pass point and cell arrays
  resample->PassCellArraysOn();
  resample->PassPointArraysOn();
  resample->Update();
  result = static_cast<svtkMultiBlockDataSet*>(resample->GetOutput());
  block = static_cast<svtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 1 ||
    block->GetCellData()->GetNumberOfArrays() != 6 ||
    block->GetPointData()->GetNumberOfArrays() != 8)
  {
    std::cout << "Unexpected number of arrays in output with pass cell and point arrays"
              << std::endl;
    return !svtkTesting::FAILED;
  }

  // don't pass field arrays
  resample->PassFieldArraysOff();
  resample->Update();
  result = static_cast<svtkMultiBlockDataSet*>(resample->GetOutput());
  block = static_cast<svtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 0 ||
    block->GetCellData()->GetNumberOfArrays() != 6 ||
    block->GetPointData()->GetNumberOfArrays() != 8)
  {
    std::cout << "Unexpected number of arrays in output with pass field arrays off" << std::endl;
    return !svtkTesting::FAILED;
  }

  // Render
  svtkNew<svtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputData(resample->GetOutputDataObject(0));

  double range[2];
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(range);

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(range);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
