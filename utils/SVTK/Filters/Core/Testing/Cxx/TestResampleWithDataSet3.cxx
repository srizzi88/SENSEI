/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset3.cxx

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
#include "svtkThreshold.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

namespace
{

void CreateInputDataSet(svtkMultiBlockDataSet* dataset, int numberOfBlocks)
{
  dataset->SetNumberOfBlocks(numberOfBlocks);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-11, 11, -11, 11, -11, 11);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-11, 11, -11, 11, -11, 11);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(10);
  cylinder->SetAxis(0, 1, 0);
  svtkNew<svtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetClipFunction(cylinder);
  clipCyl->InsideOutOn();

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(7);
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
  randomAttrs->GenerateAllCellDataOn();
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
  extentTranslator->SetWholeExtent(-17, 17, -17, 17, -11, 11);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-17, 17, -17, 17, -11, 11);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkThreshold> threshold;
  threshold->SetInputConnection(wavelet->GetOutputPort());
  threshold->ThresholdByLower(185.0);

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    threshold->Update();

    svtkDataObject* block = threshold->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(threshold->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

} // anonymous namespace

int TestResampleWithDataSet3(int argc, char* argv[])
{
  // create input dataset
  svtkNew<svtkMultiBlockDataSet> input;
  CreateInputDataSet(input, 3);

  svtkNew<svtkMultiBlockDataSet> source;
  CreateSourceDataSet(source, 4);

  svtkNew<svtkResampleWithDataSet> resample;
  resample->SetInputData(input);
  resample->SetSourceData(source);

  svtkMultiBlockDataSet* result;
  svtkDataSet* block0;

  // Test that ghost arrays are not generated
  resample->MarkBlankPointsAndCellsOff();
  resample->Update();
  result = svtkMultiBlockDataSet::SafeDownCast(resample->GetOutput());
  block0 = svtkDataSet::SafeDownCast(result->GetBlock(0));
  if (block0->GetPointGhostArray() || block0->GetCellGhostArray())
  {
    std::cout << "Error: ghost arrays were generated with MarkBlankPointsAndCellsOff()"
              << std::endl;
    return !svtkTesting::FAILED;
  }

  // Test that ghost arrays are generated
  resample->MarkBlankPointsAndCellsOn();
  resample->Update();
  result = svtkMultiBlockDataSet::SafeDownCast(resample->GetOutput());
  block0 = svtkDataSet::SafeDownCast(result->GetBlock(0));
  if (!block0->GetPointGhostArray() || !block0->GetCellGhostArray())
  {
    std::cout << "Error: no ghost arrays generated with MarkBlankPointsAndCellsOn()" << std::endl;
    return !svtkTesting::FAILED;
  }

  // Render
  double scalarRange[2];
  svtkNew<svtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputConnection(resample->GetOutputPort());
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(scalarRange);

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(scalarRange);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
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
