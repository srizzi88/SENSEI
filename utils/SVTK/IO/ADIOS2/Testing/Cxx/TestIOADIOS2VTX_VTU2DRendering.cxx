/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTU2DRendering.cxx

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
 * TestIOADIOS2VTX_VTU2DRendering.cxx : simple rendering test for unstructured
 *                                      grid data from 2D to 3D
 *
 *  Created on: Jun 19, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "svtkADIOS2VTXReader.h"

#include <numeric> //std::iota

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetMapper.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"
#include "svtkUnstructuredGrid.h"

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

void WriteBP(const std::string& fileName)
{

  // clang-format off
  const std::vector<std::uint64_t> connectivity = {
          4, 0, 1, 2, 3,
          4, 2, 3, 4, 5};

  const std::vector<double> vertices = { 0, 0,
                                         1, 0,
                                         0, 1,
                                         1, 1,
                                         0, 2,
                                         1, 2};
  // clang-format on

  std::vector<double> sol(6);
  std::iota(sol.begin(), sol.end(), 1.);

  adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);
  fs.write("types", 8);
  fs.write("connectivity", connectivity.data(), {}, {}, { 2, 5 });
  fs.write("vertices", vertices.data(), {}, {}, { 6, 2 });
  fs.write("sol", sol.data(), {}, {}, { 6 });

    const std::string vtuXML = R"(
  <SVTKFile type="UnstructuredGrid">
    <UnstructuredGrid>
      <Piece>
        <Points>
          <DataArray Name="vertices" />
        </Points>
        <Cells>
          <DataArray Name="connectivity" />
          <DataArray Name="types" />
        </Cells>
        <PointData>
          <DataArray Name="sol" />
        </PointData>
      </Piece>
    </UnstructuredGrid>
  </SVTKFile>)";

    fs.write_attribute("svtk.xml", vtuXML);
    fs.close();
}

} // end empty namespace

int TestIOADIOS2VTX_VTU2DRendering(int argc, char* argv[])
{
  svtkNew<svtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  svtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();

  svtkNew<svtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  const std::string fileName = rootDirectory + "/testVTU2D.bp";
  if (rank == 0)
  {
    WriteBP(fileName);
  }

  svtkNew<svtkADIOS2VTXReader> adios2Reader;
  adios2Reader->SetFileName(fileName.c_str());
  adios2Reader->UpdateInformation();
  adios2Reader->Update();

  svtkMultiBlockDataSet* multiBlock = adios2Reader->GetOutput();
  svtkMultiPieceDataSet* mp = svtkMultiPieceDataSet::SafeDownCast(multiBlock->GetBlock(0));
  svtkUnstructuredGrid* unstructuredGrid = svtkUnstructuredGrid::SafeDownCast(mp->GetPiece(0));

  // set color table
  svtkSmartPointer<svtkLookupTable> lookupTable = svtkSmartPointer<svtkLookupTable>::New();
  lookupTable->SetNumberOfTableValues(10);
  lookupTable->SetRange(0.0, 1.0);
  lookupTable->Build();

  // render unstructured grid
  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputData(unstructuredGrid);
  mapper->SetLookupTable(lookupTable);
  mapper->SelectColorArray("sol");
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

  mpiController->Finalize();

  return EXIT_SUCCESS;
}
