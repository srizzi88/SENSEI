/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTI3DRendering.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * TestIOADIOS2VTX_VTI3DRendering.cxx : simple rendering test
 *
 *  Created on: Jun 19, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "svtkADIOS2VTXReader.h"

#include <numeric>

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"

#include <adios2.h>

namespace
{
MPI_Comm MPIGetComm()
{
  MPI_Comm comm = MPI_COMM_NULL;
  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
  svtkMPICommunicator* svtkComm = svtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  if (svtkComm)
  {
    if (svtkComm->GetMPIComm())
    {
      comm = *(svtkComm->GetMPIComm()->GetHandle());
    }
  }

  return comm;
}

int MPIGetRank()
{
  MPI_Comm comm = MPIGetComm();
  int rank;
  MPI_Comm_rank(comm, &rank);
  return rank;
}

int MPIGetSize()
{
  MPI_Comm comm = MPIGetComm();
  int size;
  MPI_Comm_size(comm, &size);
  return size;
}

std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept
{
  return std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<std::size_t>());
}

void WriteBPFile3DVars(const std::string& fileName, const adios2::Dims& shape,
  const adios2::Dims& start, const adios2::Dims& count, const int rank)
{
  const size_t totalElements = TotalElements(count);

  const std::string extent = "0 " + std::to_string(shape[0]) + " " + "0 " +
    std::to_string(shape[1]) + " " + "0 " + std::to_string(shape[2]);

    const std::string imageSchema = R"( <?xml version="1.0"?>
      <SVTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
        <ImageData WholeExtent=")" + extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
          <Piece Extent=")" + extent +
                                    R"(">
            <CellData>
              <DataArray Name="T" />
              <DataArray Name="TIME">
                time
              </DataArray>
            </CellData>
          </Piece>
        </ImageData>
      </SVTKFile>)";

    // using adios2 C++ high-level API
    std::vector<double> T(totalElements);
    std::iota(T.begin(), T.end(), static_cast<double>(rank * totalElements));

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("svtk.xml", imageSchema);
    fw.write("time", 0);
    fw.write("T", T.data(), shape, start, count);
    fw.close();
}

} // end empty namespace

int TestIOADIOS2VTX_VTI3DRendering(int argc, char* argv[])
{
  svtkNew<svtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  svtkMultiProcessController::SetGlobalController(mpiController);

  const int rank = MPIGetRank();
  const int size = MPIGetSize();

  svtkNew<svtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  const std::string fileName = rootDirectory + "/heat3D_render.bp";
  const adios2::Dims count{ 4, 4, 8 };
  const adios2::Dims start{ static_cast<size_t>(rank) * count[0], 0, 0 };
  const adios2::Dims shape{ static_cast<size_t>(size) * count[0], count[1], count[2] };

  WriteBPFile3DVars(fileName, shape, start, count, rank);

  svtkNew<svtkADIOS2VTXReader> adios2Reader;
  adios2Reader->SetFileName(fileName.c_str());
  adios2Reader->UpdateInformation();
  adios2Reader->Update();

  svtkMultiBlockDataSet* multiBlock = adios2Reader->GetOutput();
  svtkMultiPieceDataSet* mp = svtkMultiPieceDataSet::SafeDownCast(multiBlock->GetBlock(0));
  svtkImageData* imageData = svtkImageData::SafeDownCast(mp->GetPiece(rank));

  if (false)
  {
    double* data =
      reinterpret_cast<double*>(imageData->GetCellData()->GetArray("T")->GetVoidPointer(0));

    for (size_t i = 0; i < 128; ++i)
    {
      if (data[i] != static_cast<double>(i))
      {
        throw std::invalid_argument("ERROR: invalid source data for rendering\n");
      }
    }
  }
  // set color table
  svtkSmartPointer<svtkLookupTable> lookupTable = svtkSmartPointer<svtkLookupTable>::New();
  lookupTable->SetNumberOfTableValues(10);
  lookupTable->SetRange(0.0, 1.0);
  lookupTable->Build();

  // render imageData
  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputData(imageData);
  mapper->SetLookupTable(lookupTable);
  // mapper->SelectColorArray("T");
  mapper->SetScalarModeToUseCellFieldData();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  // Add both renderers to the window
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindow->Render();
  // renderWindowInteractor->Start();

  mpiController->Finalize();

  return EXIT_SUCCESS;
}
