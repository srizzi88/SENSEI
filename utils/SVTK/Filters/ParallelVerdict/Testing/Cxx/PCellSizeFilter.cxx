#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPCellSizeFilter.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

int PCellSizeFilter(int argc, char* argv[])
{
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv);
  contr->SetGlobalController(contr);
  contr->CreateOutputWindow();

  svtkNew<svtkUnstructuredGridReader> reader;
  svtkNew<svtkCellSizeFilter> filter;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.svtk");

  reader->SetFileName(fname);
  delete[] fname;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->ComputeSumOn();
  filter->Update();

  svtkDoubleArray* vertexCount = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("VertexCount"));
  svtkDoubleArray* length = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Length"));
  svtkDoubleArray* area = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Area"));
  svtkDoubleArray* volume = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Volume"));
  svtkDoubleArray* arrays[4] = { vertexCount, length, area, volume };

  // types are hex, hex, tet, tet, polygon, triangle-strip, quad, triangle,
  // triangle, line, line, vertex
  double correctValues[12] = { 1, 1, .16667, .16667, 2, 2, 1, .5, .5, 1, 1, 1 };
  int dimensions[12] = { 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 0 };
  for (int i = 0; i < 4; i++)
  {
    if (!arrays[i])
    {
      svtkGenericWarningMacro(
        "Cannot find expected array output for dimension " << i << " from svtkCellSizeFilter");
      return EXIT_FAILURE;
    }
    for (svtkIdType j = 0; j < arrays[i]->GetNumberOfTuples(); j++)
    {
      if (dimensions[j] == i && fabs(arrays[i]->GetValue(j) - correctValues[j]) > .0001)
      {
        svtkGenericWarningMacro("Wrong size for cell " << j);
        return EXIT_FAILURE;
      }
    }
  }
  double correctSumValues[4] = { correctValues[11], correctValues[10] + correctValues[9],
    correctValues[8] + correctValues[7] + correctValues[6] + correctValues[5] + correctValues[4],
    correctValues[3] + correctValues[2] + correctValues[1] + correctValues[0] };

  vertexCount = svtkDoubleArray::SafeDownCast(svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
                                               ->GetFieldData()
                                               ->GetArray("VertexCount"));
  if (fabs(vertexCount->GetValue(0) - correctSumValues[0]) > .0001)
  {
    svtkGenericWarningMacro("Wrong size sum for dimension 0");
    return EXIT_FAILURE;
  }
  length = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Length"));
  if (fabs(length->GetValue(0) - correctSumValues[1]) > .0001)
  {
    svtkGenericWarningMacro("Wrong size sum for dimension 1");
    return EXIT_FAILURE;
  }
  area = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Area"));
  if (fabs(area->GetValue(0) - correctSumValues[2]) > .0001)
  {
    svtkGenericWarningMacro("Wrong size sum for dimension 2");
    return EXIT_FAILURE;
  }
  volume = svtkDoubleArray::SafeDownCast(
    svtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Volume"));
  if (fabs(volume->GetValue(0) - correctSumValues[3]) > .0001)
  {
    svtkGenericWarningMacro("Wrong size sum for dimension 3");
    return EXIT_FAILURE;
  }

  contr->Finalize();
  contr->Delete();

  return EXIT_SUCCESS;
}
