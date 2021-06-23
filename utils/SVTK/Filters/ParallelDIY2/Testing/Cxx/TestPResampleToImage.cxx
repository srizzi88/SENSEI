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
#include "svtkCamera.h"
#include "svtkClipDataSet.h"
#include "svtkCompositeRenderManager.h"
#include "svtkContourFilter.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPExtractVOI.h"
#include "svtkPieceScalars.h"
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

int TestPResampleToImage(int argc, char* argv[])
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

  // Create Pipeline
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 31, 0, 31, 0, 31);
  wavelet->SetCenter(16, 16, 16);

  svtkNew<svtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  svtkNew<svtkPResampleToImage> resample;
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(64, 64, 64);
  resample->SetInputConnection(clip->GetOutputPort());

  svtkNew<svtkPExtractVOI> voi;
  voi->SetVOI(4, 59, 4, 59, 4, 59);
  voi->SetInputConnection(resample->GetOutputPort());

  svtkNew<svtkContourFilter> contour;
  contour->SetValue(0, 200);
  contour->ComputeNormalsOn();
  contour->SetInputConnection(voi->GetOutputPort());

  svtkNew<svtkPieceScalars> pieceScalars;
  pieceScalars->SetInputConnection(contour->GetOutputPort());
  pieceScalars->SetScalarModeToCellData();

  // Execute pipeline and render
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pieceScalars->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, world.size() - 1);
  mapper->SetPiece(world.rank());
  mapper->SetNumberOfPieces(world.size());
  mapper->Update();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal;
  if (world.rank() == 0)
  {
    prm->ResetAllCameras();
    renderer->GetActiveCamera()->Azimuth(90);

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
