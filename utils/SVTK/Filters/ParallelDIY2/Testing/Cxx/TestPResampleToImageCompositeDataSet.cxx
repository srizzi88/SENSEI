/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPResampleToImage.h"

#include "svtkActor.h"
#include "svtkAssignAttribute.h"
#include "svtkCamera.h"
#include "svtkCompositeRenderManager.h"
#include "svtkContourFilter.h"
#include "svtkExtentTranslator.h"
#include "svtkImageData.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointDataToCellData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

// clang-format off
#include "svtk_diy2.h" // must include this before any diy header
#include SVTK_DIY2(diy/mpi.hpp)
// clang-format on

int TestPResampleToImageCompositeDataSet(int argc, char* argv[])
{
  diy::mpi::environment mpienv(argc, argv);
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv, true);
  diy::mpi::communicator world;

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

  // Create input dataset
  const int piecesPerRank = 2;
  int numberOfPieces = world.size() * piecesPerRank;

  svtkNew<svtkMultiBlockDataSet> input;
  input->SetNumberOfBlocks(numberOfPieces);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(0, 31, 0, 31, 0, 31);
  extentTranslator->SetNumberOfPieces(numberOfPieces);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 31, 0, 31, 0, 31);
  wavelet->SetCenter(16, 16, 16);
  svtkNew<svtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(wavelet->GetOutputPort());

  for (int i = 0; i < piecesPerRank; ++i)
  {
    int piece = (world.rank() * piecesPerRank) + i;
    int pieceExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(pieceExtent);
    pointToCell->UpdateExtent(pieceExtent);
    svtkNew<svtkImageData> img;
    img->DeepCopy(svtkImageData::SafeDownCast(pointToCell->GetOutput()));
    input->SetBlock(piece, img);
  }

  // create pipeline
  svtkNew<svtkPResampleToImage> resample;
  resample->SetInputDataObject(input);
  resample->SetController(controller);
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(64, 64, 64);

  svtkNew<svtkAssignAttribute> assignAttrib;
  assignAttrib->SetInputConnection(resample->GetOutputPort());
  assignAttrib->Assign("RTData", svtkDataSetAttributes::SCALARS, svtkAssignAttribute::POINT_DATA);

  svtkNew<svtkContourFilter> contour;
  contour->SetInputConnection(assignAttrib->GetOutputPort());
  contour->SetValue(0, 157);
  contour->ComputeNormalsOn();

  // Execute pipeline and render
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->Update();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal;
  if (world.rank() == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();
    retVal = svtkRegressionTester::Test(argc, argv, renWin, 10);
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
  world.barrier();

  diy::mpi::broadcast(world, retVal, 0);

  controller->Finalize(true);

  return !retVal;
}
