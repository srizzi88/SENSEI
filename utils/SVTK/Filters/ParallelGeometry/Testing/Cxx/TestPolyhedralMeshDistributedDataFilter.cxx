#include "svtkDistributedDataFilter.h"
#include "svtkMPIController.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"

#include <string>

void AbortTest(svtkMPIController* controller, const std::string& message)
{
  if (controller->GetLocalProcessId() == 0)
  {
    svtkErrorWithObjectMacro(nullptr, << message.c_str());
  }
  controller->Finalize();
  exit(EXIT_FAILURE);
}

int TestPolyhedralMeshDistributedDataFilter(int argc, char* argv[])
{
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);

  const int rank = controller->GetLocalProcessId();

  svtkMultiProcessController::SetGlobalController(controller);

  svtkSmartPointer<svtkUnstructuredGrid> ug;
  // Load a full polyhedral mesh on rank 0, others have an empty piece
  if (rank == 0)
  {
    svtkNew<svtkXMLUnstructuredGridReader> r;
    char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/voronoiMesh.vtu");
    r->SetFileName(fname);
    r->Update();
    ug = r->GetOutput();
    delete[] fname;
  }
  else
  {
    ug = svtkSmartPointer<svtkUnstructuredGrid>::New();
  }

  // Fetch number of cells of the full distributed mesh
  svtkIdType allNbCells = 0;
  svtkIdType localNbCells = ug->GetNumberOfCells();
  controller->AllReduce(&localNbCells, &allNbCells, 1, svtkCommunicator::SUM_OP);
  if (allNbCells == 0)
  {
    AbortTest(controller, "ERROR: Check grid failed!");
  }

  // Distribute the mesh with the D3 filter
  svtkNew<svtkDistributedDataFilter> d3;
  d3->SetInputData(ug);
  d3->SetController(controller);
  d3->SetBoundaryMode(0);
  d3->SetUseMinimalMemory(0);
  d3->SetMinimumGhostLevel(0);
  d3->Update();

  ug = svtkUnstructuredGrid::SafeDownCast(d3->GetOutput());

  // Check that each rank own a piece of the full mesh
  int success = ug->GetNumberOfCells() != 0 ? 1 : 0;
  int allsuccess = 0;
  controller->AllReduce(&success, &allsuccess, 1, svtkCommunicator::MIN_OP);
  if (allsuccess == 0)
  {
    AbortTest(controller, "ERROR: Invalid mesh distribution - some ranks have 0 cell.");
  }

  // Check that input and output distributed meshes have same number of cells
  svtkIdType allOutNbCells = 0;
  svtkIdType localOutNbCells = ug->GetNumberOfCells();
  controller->AllReduce(&localOutNbCells, &allOutNbCells, 1, svtkCommunicator::SUM_OP);
  if (allNbCells != allOutNbCells)
  {
    AbortTest(controller,
      "ERROR: Invalid mesh distribution - input and output mesh have different number of cells.");
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
