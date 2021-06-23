/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRedistributeDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

===========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkExodusIIReader.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPartitionedDataSet.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRedistributeDataSetFilter.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
#include "svtkMPIController.h"
#else
#include "svtkDummyController.h"
#endif

// clang-format off
#include "svtk_diy2.h"
#include SVTK_DIY2(diy/mpi.hpp)
// clang-format on

namespace
{
bool ValidateDataset(
  svtkUnstructuredGrid* input, svtkPartitionedDataSet* output, svtkMultiProcessController* controller)
{
  const int rank = controller->GetLocalProcessId();
  svtkIdType local_cellid_max = 0;
  for (unsigned int part = 0; part < output->GetNumberOfPartitions(); ++part)
  {
    if (auto ds = svtkDataSet::SafeDownCast(output->GetPartition(part)))
    {
      if (auto gcids = svtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds()))
      {
        local_cellid_max =
          std::max(static_cast<svtkIdType>(gcids->GetRange(0)[1]), local_cellid_max);
      }
    }
  }

  svtkIdType global_cellid_max;
  controller->AllReduce(&local_cellid_max, &global_cellid_max, 1, svtkCommunicator::MAX_OP);
  if (rank == 0 && global_cellid_max != input->GetNumberOfCells() - 1)
  {
    svtkLogF(ERROR, "incorrect global cell ids! expected %lld, actual %lld",
      input->GetNumberOfCells() - 1, global_cellid_max);
    return false;
  }

  return true;
}

}

int TestRedistributeDataSetFilter(int argc, char* argv[])
{
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  svtkNew<svtkMPIController> controller;
#else
  svtkNew<svtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  svtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  svtkLogger::SetThreadName("rank:" + std::to_string(rank));

  svtkSmartPointer<svtkUnstructuredGrid> data;
  if (rank == 0)
  {
    char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
    if (!fname)
    {
      svtkLogF(ERROR, "Could not obtain filename for test data.");
      return EXIT_FAILURE;
    }

    svtkNew<svtkExodusIIReader> rdr;
    if (!rdr->CanReadFile(fname))
    {
      svtkLogF(ERROR, "Cannot read `%s`", fname);
      return 1;
    }

    rdr->SetFileName(fname);
    delete[] fname;
    rdr->Update();

    data = svtkUnstructuredGrid::SafeDownCast(
      svtkMultiBlockDataSet::SafeDownCast(rdr->GetOutput()->GetBlock(0))->GetBlock(0));
  }
  else
  {
    data = svtkSmartPointer<svtkUnstructuredGrid>::New();
  }

  svtkNew<svtkRedistributeDataSetFilter> rdsf;
  rdsf->SetInputDataObject(data);
  rdsf->SetNumberOfPartitions(16);
  rdsf->GenerateGlobalCellIdsOn();
  rdsf->PreservePartitionsInOutputOn();
  rdsf->Update();

  if (!ValidateDataset(
        data, svtkPartitionedDataSet::SafeDownCast(rdsf->GetOutputDataObject(0)), controller))
  {
    return EXIT_FAILURE;
  }

  svtkNew<svtkDataSetSurfaceFilter> dsf;
  dsf->SetInputConnection(rdsf->GetOutputPort());

  svtkNew<svtkRandomAttributeGenerator> rag;
  rag->SetDataTypeToDouble();
  rag->SetNumberOfComponents(1);
  rag->SetComponentRange(0, 1.0);
  rag->GenerateCellScalarsOn();
  rag->AttributesConstantPerBlockOn();
  rag->SetInputConnection(dsf->GetOutputPort());

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(rag->GetOutputPort());

  svtkNew<svtkCompositeRenderManager> prm;
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::Take(prm->MakeRenderer());
  svtkSmartPointer<svtkRenderWindow> renWin =
    svtkSmartPointer<svtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal = 1;
  if (rank == 0)
  {
    prm->ResetAllCameras();

    if (auto camera = renderer->GetActiveCamera())
    {
      camera->SetFocalPoint(-0.531007, -1.16954, -1.12284);
      camera->SetPosition(8.62765, 28.0586, -33.585);
      camera->SetViewUp(-0.373065, 0.739388, 0.560472);
    }

    renWin->Render();

    retVal = svtkRegressionTestImage(renWin);
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

  controller->Broadcast(&retVal, 1, 0);
  controller->Finalize();
  svtkMultiProcessController::SetGlobalController(nullptr);
  return !retVal;
}
