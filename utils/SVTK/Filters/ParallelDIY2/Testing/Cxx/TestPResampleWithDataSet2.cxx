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

#include "svtkPResampleWithDataSet.h"

#include "svtkActor.h"
#include "svtkBoundingBox.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkCompositeRenderManager.h"
#include "svtkCylinder.h"
#include "svtkExtentTranslator.h"
#include "svtkImageData.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkThreshold.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

#include <algorithm>
#include <cmath>

namespace
{

void CreateSourceDataSet(
  svtkMultiBlockDataSet* dataset, int rank, int numberOfProcs, int blocksPerProc)
{
  int numPieces = blocksPerProc * numberOfProcs;
  dataset->SetNumberOfBlocks(numPieces);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  extentTranslator->SetNumberOfPieces(numPieces);
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

  for (int i = 0; i < blocksPerProc; ++i)
  {
    int piece = (rank * blocksPerProc) + i;

    int blockExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    clipCyl->SetInputData(wavelet->GetOutputDataObject(0));
    transFilter->Update();

    svtkDataObject* block = transFilter->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(transFilter->GetOutputDataObject(0));
    dataset->SetBlock(piece, block);
    block->Delete();
  }
}

void CreateInputDataSet(svtkMultiBlockDataSet* dataset, const double bounds[6], int rank,
  int numberOfProcs, int numberOfBlocks)
{
  static const int dims[] = { 96, 32, 64 };
  dataset->SetNumberOfBlocks(numberOfBlocks);

  double size[3] = { bounds[1] - bounds[0], bounds[3] - bounds[2],
    (bounds[5] - bounds[4]) / static_cast<double>(numberOfBlocks) };
  for (int i = 0; i < numberOfBlocks; ++i)
  {
    double origin[3] = { bounds[0], bounds[2], bounds[4] + static_cast<double>(i) * size[2] };
    double spacing = (*std::max_element(size, size + 3)) / static_cast<double>(dims[i % 3]);

    int extent[6];
    extent[0] = 0;
    extent[1] = static_cast<int>(size[0] / spacing) - 1;
    extent[2] = rank * (static_cast<int>(size[1] / spacing) / numberOfProcs);
    extent[3] = extent[2] + (static_cast<int>(size[1] / spacing) / numberOfProcs);
    extent[4] = 0;
    extent[5] = static_cast<int>(std::ceil(size[2] / spacing));

    svtkNew<svtkImageData> img;
    img->SetExtent(extent);
    img->SetOrigin(origin);
    img->SetSpacing(spacing, spacing, spacing);
    dataset->SetBlock(i, img);
  }
}

void ComputeGlobalBounds(
  svtkMultiBlockDataSet* dataset, svtkMultiProcessController* controller, double bounds[6])
{
  svtkBoundingBox bb;
  for (unsigned i = 0; i < dataset->GetNumberOfBlocks(); ++i)
  {
    svtkDataSet* block = svtkDataSet::SafeDownCast(dataset->GetBlock(i));
    if (block)
    {
      bb.AddBounds(block->GetBounds());
    }
  }

  double lbmin[3], lbmax[3];
  bb.GetBounds(lbmin[0], lbmax[0], lbmin[1], lbmax[1], lbmin[2], lbmax[2]);

  double gbmin[3], gbmax[3];
  controller->AllReduce(lbmin, gbmin, 3, svtkCommunicator::MIN_OP);
  controller->AllReduce(lbmax, gbmax, 3, svtkCommunicator::MAX_OP);

  for (int i = 0; i < 3; ++i)
  {
    bounds[2 * i] = gbmin[i];
    bounds[2 * i + 1] = gbmax[i];
  }
}

} // anonymous namespace

int TestPResampleWithDataSet2(int argc, char* argv[])
{
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  int numProcs = controller->GetNumberOfProcesses();
  int rank = controller->GetLocalProcessId();

  // create source and input datasets
  svtkNew<svtkMultiBlockDataSet> source;
  CreateSourceDataSet(source, rank, numProcs, 5);

  // compute full bounds of source dataset
  double bounds[6];
  ComputeGlobalBounds(source, controller, bounds);

  svtkNew<svtkMultiBlockDataSet> input;
  CreateInputDataSet(input, bounds, rank, numProcs, 3);

  svtkNew<svtkPResampleWithDataSet> resample;
  resample->SetController(controller);
  resample->SetInputData(input);
  resample->SetSourceData(source);
  resample->Update();

  // Render
  svtkNew<svtkThreshold> threshold;
  threshold->SetInputConnection(resample->GetOutputPort());
  threshold->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "svtkValidPointMask");
  threshold->ThresholdByUpper(1);

  svtkNew<svtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputConnection(threshold->GetOutputPort());

  double range[2];
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(range);

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(range);

  // Setup parallel rendering
  svtkNew<svtkCompositeRenderManager> prm;
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::Take(prm->MakeRenderer());
  svtkSmartPointer<svtkRenderWindow> renWin =
    svtkSmartPointer<svtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal;
  if (rank == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();
    retVal = svtkRegressionTester::Test(argc, argv, renWin, 20);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    prm->StartServices();
  }
  controller->Barrier();

  controller->Broadcast(&retVal, 1, 0);
  controller->Finalize();

  return !retVal;
}
