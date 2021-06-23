/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTU3DRendering.cxx

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
 * TestIOADIOS2VTX_VTU3DRendering.cxx : simple rendering test for unstructured
 *                                      grid data
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
  const std::vector<std::uint64_t> connectivity = { 8, 0, 12, 32, 15, 20, 33, 43, 36, 8, 1, 24, 38, 13,
    21, 39, 44, 34, 8, 12, 1, 13, 32, 33, 21, 34, 43, 8, 32, 13, 4, 14, 43, 34, 22, 35, 8, 15, 32,
    14, 3, 36, 43, 35, 23, 8, 20, 33, 43, 36, 6, 16, 37, 19, 8, 33, 21, 34, 43, 16, 7, 17, 37, 8,
    43, 34, 22, 35, 37, 17, 10, 18, 8, 36, 43, 35, 23, 19, 37, 18, 9, 8, 24, 2, 25, 38, 39, 30, 40,
    44, 8, 38, 25, 5, 26, 44, 40, 31, 41, 8, 13, 38, 26, 4, 34, 44, 41, 22, 8, 21, 39, 44, 34, 7,
    27, 42, 17, 8, 39, 30, 40, 44, 27, 8, 28, 42, 8, 44, 40, 31, 41, 42, 28, 11, 29, 8, 34, 44, 41,
    22, 17, 42, 29, 10 };

  const std::vector<double> vertices = { 3.98975, -0.000438888, -0.0455599, 4.91756, -0.0080733,
    -0.149567, 5.86422, -0.00533255, -0.38101, 3.98975, 1.00044, -0.0455599, 4.91756, 1.00807,
    -0.149567, 5.86422, 1.00533, -0.38101, 4.01025, 0.000438888, 0.95444, 5.08244, 0.0080733,
    0.850433, 6.13578, 0.00533255, 0.61899, 4.01025, 0.999561, 0.95444, 5.08244, 0.991927, 0.850433,
    6.13578, 0.994667, 0.61899, 4.45173, -0.00961903, -0.0802818, 4.91711, 0.5, -0.153657, 4.45173,
    1.00962, -0.0802818, 3.98987, 0.5, -0.0457531, 4.54827, 0.00961903, 0.919718, 5.08289, 0.5,
    0.846343, 4.54827, 0.990381, 0.919718, 4.01013, 0.5, 0.954247, 4, 1.17739e-13, 0.454655, 5,
    3.36224e-12, 0.354149, 5, 1, 0.354149, 4, 1, 0.454655, 5.38824, -0.00666013, -0.252066, 5.86382,
    0.5, -0.383679, 5.38824, 1.00666, -0.252066, 5.61176, 0.00666013, 0.747934, 6.13618, 0.5,
    0.616321, 5.61176, 0.99334, 0.747934, 6, -1.7895e-12, 0.121648, 6, 1, 0.121648, 4.4528, 0.5,
    -0.0845428, 4.5, -1.95761e-12, 0.425493, 5, 0.5, 0.350191, 4.5, 1, 0.425493, 4, 0.5, 0.454445,
    4.5472, 0.5, 0.915457, 5.38782, 0.5, -0.255387, 5.5, 6.97152e-13, 0.251323, 6, 0.5, 0.118984,
    5.5, 1, 0.251323, 5.61218, 0.5, 0.744613, 4.5, 0.5, 0.421259, 5.5, 0.5, 0.247968 };

  // clang-format on

  std::vector<double> sol(45);
  std::iota(sol.begin(), sol.end(), 1.);

  adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);
  fs.write("types", 11);
  fs.write("connectivity", connectivity.data(), {}, {}, { 16, 9 });
  fs.write("vertices", vertices.data(), {}, {}, { 45, 3 });
  fs.write("sol", sol.data(), {}, {}, { 45 });

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

int TestIOADIOS2VTX_VTU3DRendering(int argc, char* argv[])
{
  svtkNew<svtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  svtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();

  svtkNew<svtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  const std::string fileName = rootDirectory + "/testVTU3D.bp";
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
