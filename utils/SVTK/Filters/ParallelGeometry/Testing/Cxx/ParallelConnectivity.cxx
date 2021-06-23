/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkConnectivityFilter.h"

#include "svtkCellData.h"
#include "svtkContourFilter.h"
#include "svtkDataSetTriangleFilter.h"
#include "svtkDistributedDataFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkMPIController.h"
#include "svtkPConnectivityFilter.h"
#include "svtkPUnstructuredGridGhostCellsGenerator.h"
#include "svtkRemoveGhosts.h"
#include "svtkStructuredPoints.h"
#include "svtkStructuredPointsReader.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

#include <svtk_mpi.h>

int RunParallelConnectivity(
  const char* fname, svtkAlgorithm::DesiredOutputPrecision precision, svtkMPIController* contr)
{
  int returnValue = EXIT_SUCCESS;
  int me = contr->GetLocalProcessId();

  svtkNew<svtkStructuredPointsReader> reader;
  svtkDataSet* ds;
  svtkSmartPointer<svtkUnstructuredGrid> ug = svtkSmartPointer<svtkUnstructuredGrid>::New();
  if (me == 0)
  {
    std::cout << fname << std::endl;
    reader->SetFileName(fname);
    reader->Update();

    ds = reader->GetOutput();
  }
  else
  {
    ds = ug;
  }

  svtkNew<svtkDistributedDataFilter> dd;
  dd->SetInputData(ds);
  dd->SetController(contr);
  dd->UseMinimalMemoryOff();
  dd->SetBoundaryModeToAssignToOneRegion();

  svtkNew<svtkContourFilter> contour;
  contour->SetInputConnection(dd->GetOutputPort());
  contour->SetNumberOfContours(1);
  contour->SetOutputPointsPrecision(precision);
  contour->SetValue(0, 240.0);

  svtkNew<svtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(contour->GetOutputPort());

  svtkNew<svtkPUnstructuredGridGhostCellsGenerator> ghostCells;
  ghostCells->SetController(contr);
  ghostCells->SetBuildIfRequired(false);
  ghostCells->SetMinimumNumberOfGhostLevels(1);
  ghostCells->SetInputConnection(tetrahedralize->GetOutputPort());

  // Test factory override mechanism instantiated as a svtkPConnectivityFilter.
  svtkNew<svtkConnectivityFilter> connectivity;
  if (!connectivity->IsA("svtkPConnectivityFilter"))
  {
    std::cerr << "Expected svtkConnectivityFilter filter to be instantiated "
              << "as a svtkPConnectivityFilter with MPI support enabled, but "
              << "it is a " << connectivity->GetClassName() << " instead." << std::endl;
  }

  connectivity->SetInputConnection(ghostCells->GetOutputPort());
  connectivity->Update();

  // Remove ghost points/cells so that the cell count is the same regardless
  // of the number of processes.
  svtkNew<svtkRemoveGhosts> removeGhosts;
  removeGhosts->SetInputConnection(connectivity->GetOutputPort());

  // Check the number of regions
  int numberOfRegions = connectivity->GetNumberOfExtractedRegions();
  int expectedNumberOfRegions = 19;
  if (numberOfRegions != expectedNumberOfRegions)
  {
    std::cerr << "Expected " << expectedNumberOfRegions << " regions but got " << numberOfRegions
              << std::endl;
    returnValue = EXIT_FAILURE;
  }

  // Check that assigning RegionIds by number of cells (descending) works
  connectivity->SetRegionIdAssignmentMode(svtkConnectivityFilter::CELL_COUNT_DESCENDING);
  connectivity->ColorRegionsOn();
  connectivity->SetExtractionModeToAllRegions();
  removeGhosts->Update();
  numberOfRegions = connectivity->GetNumberOfExtractedRegions();
  svtkPointSet* ghostOutput = svtkPointSet::SafeDownCast(removeGhosts->GetOutput());
  svtkIdType numberOfCells = ghostOutput->GetNumberOfCells();
  svtkIdType globalNumberOfCells = 0;
  contr->AllReduce(&numberOfCells, &globalNumberOfCells, 1, svtkCommunicator::SUM_OP);
  std::vector<svtkIdType> regionCounts(connectivity->GetNumberOfExtractedRegions(), 0);

  // Count up cells with RegionIds
  auto regionIdArray =
    svtkIdTypeArray::SafeDownCast(ghostOutput->GetCellData()->GetArray("RegionId"));
  for (svtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    svtkIdType regionId = regionIdArray->GetValue(cellId);
    regionCounts[regionId]++;
  }

  // Sum up region counts across processes
  std::vector<svtkIdType> globalRegionCounts(regionCounts.size(), 0);
  contr->AllReduce(regionCounts.data(), globalRegionCounts.data(),
    static_cast<svtkIdType>(regionCounts.size()), svtkCommunicator::SUM_OP);
  if (me == 0)
  {
    bool printCounts = false;
    for (svtkIdType i = 1; i < numberOfRegions; ++i)
    {
      if (globalRegionCounts[i] > globalRegionCounts[i - 1])
      {
        std::cerr << "Region " << i << " is larger than region " << i - 1 << std::endl;
        printCounts = true;
        returnValue = EXIT_FAILURE;
        break;
      }
    }
    if (printCounts)
    {
      for (svtkIdType i = 0; i < numberOfRegions; ++i)
      {
        std::cout << "Region " << i << " has " << globalRegionCounts[i] << " cells" << std::endl;
      }
    }
  }

  // Check that assignment RegionIds by number of cells (ascending) works
  connectivity->SetRegionIdAssignmentMode(svtkConnectivityFilter::CELL_COUNT_ASCENDING);
  removeGhosts->Update();

  std::fill(regionCounts.begin(), regionCounts.end(), 0);
  regionIdArray = svtkIdTypeArray::SafeDownCast(ghostOutput->GetCellData()->GetArray("RegionId"));
  for (svtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    svtkIdType regionId = regionIdArray->GetValue(cellId);
    regionCounts[regionId]++;
  }

  // Sum up region counts across processes
  globalRegionCounts = std::vector<svtkIdType>(regionCounts.size(), 0);
  contr->AllReduce(regionCounts.data(), globalRegionCounts.data(),
    static_cast<svtkIdType>(regionCounts.size()), svtkCommunicator::SUM_OP);
  if (me == 0)
  {
    bool printCounts = false;
    for (svtkIdType i = 1; i < numberOfRegions; ++i)
    {
      if (globalRegionCounts[i] < globalRegionCounts[i - 1])
      {
        std::cerr << "Region " << i << " is smaller than " << i - 1 << std::endl;
        printCounts = true;
        returnValue = EXIT_FAILURE;
        break;
      }
    }
    if (printCounts)
    {
      for (svtkIdType i = 0; i < numberOfRegions; ++i)
      {
        std::cout << "Region " << i << " has " << globalRegionCounts[i] << " cells" << std::endl;
      }
    }
  }

  // Check the number of cells in the largest region when the extraction mode
  // is set to largest region.
  connectivity->SetExtractionModeToLargestRegion();
  removeGhosts->Update();
  numberOfCells = svtkPointSet::SafeDownCast(removeGhosts->GetOutput())->GetNumberOfCells();
  globalNumberOfCells = 0;
  contr->AllReduce(&numberOfCells, &globalNumberOfCells, 1, svtkCommunicator::SUM_OP);

  int expectedNumberOfCells = 2124;
  if (globalNumberOfCells != expectedNumberOfCells)
  {
    std::cerr << "Expected " << expectedNumberOfCells << " cells in largest "
              << "region bug got " << globalNumberOfCells << std::endl;
    returnValue = EXIT_FAILURE;
  }

  // Closest point region test
  connectivity->SetExtractionModeToClosestPointRegion();
  removeGhosts->Update();
  numberOfCells = svtkPointSet::SafeDownCast(removeGhosts->GetOutput())->GetNumberOfCells();
  contr->AllReduce(&numberOfCells, &globalNumberOfCells, 1, svtkCommunicator::SUM_OP);
  expectedNumberOfCells = 862; // point (0, 0, 0)
  if (globalNumberOfCells != expectedNumberOfCells)
  {
    std::cerr << "Expected " << expectedNumberOfCells << " cells in closest "
              << "point extraction mode but got " << globalNumberOfCells << std::endl;
    returnValue = EXIT_FAILURE;
  }

  return returnValue;
}

int ParallelConnectivity(int argc, char* argv[])
{
  int returnValue = EXIT_SUCCESS;

  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  svtkMultiProcessController::SetGlobalController(contr);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.svtk");

  if (RunParallelConnectivity(fname, svtkAlgorithm::SINGLE_PRECISION, contr) != EXIT_SUCCESS)
  {
    std::cerr << "Error running with svtkAlgorithm::SINGLE_PRECISION" << std::endl;
    returnValue = EXIT_FAILURE;
  }
  if (RunParallelConnectivity(fname, svtkAlgorithm::DOUBLE_PRECISION, contr) != EXIT_SUCCESS)
  {
    std::cerr << "Error running with svtkAlgorithm::DOUBLE_PRECISION" << std::endl;
    returnValue = EXIT_FAILURE;
  }

  delete[] fname;

  contr->Finalize();
  contr->Delete();

  return returnValue;
}
