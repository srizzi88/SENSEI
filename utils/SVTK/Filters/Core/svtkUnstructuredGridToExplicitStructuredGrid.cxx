/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnstructuredGridToExplicitStructuredGrid.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkUnstructuredGridToExplicitStructuredGrid);

//-----------------------------------------------------------------------------
svtkUnstructuredGridToExplicitStructuredGrid::svtkUnstructuredGridToExplicitStructuredGrid()
{
  this->WholeExtent[0] = this->WholeExtent[1] = this->WholeExtent[2] = this->WholeExtent[3] =
    this->WholeExtent[4] = this->WholeExtent[5] = 0;
}

// ----------------------------------------------------------------------------
int svtkUnstructuredGridToExplicitStructuredGrid::RequestInformation(
  svtkInformation* svtkNotUsed(request), svtkInformationVector** svtkNotUsed(inputVector),
  svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkUnstructuredGridToExplicitStructuredGrid::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve input and output
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::GetData(inputVector[0], 0);
  svtkExplicitStructuredGrid* output = svtkExplicitStructuredGrid::GetData(outputVector, 0);

  if (!input)
  {
    svtkErrorMacro("No input!");
    return 0;
  }
  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  svtkDataArray* iArray = GetInputArrayToProcess(0, input);
  svtkDataArray* jArray = GetInputArrayToProcess(1, input);
  svtkDataArray* kArray = GetInputArrayToProcess(2, input);
  if (!iArray || !jArray || !kArray)
  {
    svtkErrorMacro("An ijk array has not be set using SetInputArrayToProcess, aborting.");
    return 0;
  }

  int extents[6];
  double r[2];
  iArray->GetRange(r);
  extents[0] = static_cast<int>(std::floor(r[0]));
  extents[1] = static_cast<int>(std::floor(r[1] + 1));
  jArray->GetRange(r);
  extents[2] = static_cast<int>(std::floor(r[0]));
  extents[3] = static_cast<int>(std::floor(r[1] + 1));
  kArray->GetRange(r);
  extents[4] = static_cast<int>(std::floor(r[0]));
  extents[5] = static_cast<int>(std::floor(r[1] + 1));

  svtkIdType expectedCells =
    (extents[1] - extents[0]) * (extents[3] - extents[2]) * (extents[5] - extents[4]);

  // Copy input point data to output
  output->GetCellData()->CopyAllocate(input->GetCellData(), expectedCells);
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->SetPoints(input->GetPoints());
  output->SetExtent(extents);

  svtkIdType nbCells = input->GetNumberOfCells();
  svtkNew<svtkCellArray> cells;
  output->SetCells(cells.Get());

  // Initialize the cell array
  cells->AllocateEstimate(expectedCells, 8);
  svtkIdType ids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (svtkIdType i = 0; i < expectedCells; i++)
  {
    cells->InsertNextCell(8, ids);
    if (expectedCells != nbCells)
    {
      output->GetCellData()->CopyData(input->GetCellData(), 0, i);
      // Blank after copying the cell data to ensure it is not overwrited
      output->BlankCell(i);
    }
  }

  int progressCount = 0;
  int abort = 0;
  svtkIdType progressInterval = nbCells / 20 + 1;

  // Copy unstructured cells
  for (svtkIdType i = 0; i < nbCells && !abort; i++)
  {
    if (progressCount >= progressInterval)
    {
      svtkDebugMacro("Process cell #" << i);
      this->UpdateProgress(static_cast<double>(i) / nbCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
    }
    progressCount++;

    int cellType = input->GetCellType(i);
    if (cellType != SVTK_HEXAHEDRON && cellType != SVTK_VOXEL)
    {
      svtkErrorMacro("Cell " << i << " is of type " << cellType << " while "
                            << "hexahedron or voxel is expected!");
      continue;
    }

    // Compute the structured cell index from IJK indices
    svtkIdType cellId = output->ComputeCellId(static_cast<int>(std::floor(iArray->GetTuple1(i))),
      static_cast<int>(std::floor(jArray->GetTuple1(i))),
      static_cast<int>(std::floor(kArray->GetTuple1(i))));
    if (cellId < 0)
    {
      svtkErrorMacro("Incorrect CellId, something went wrong");
      return 0;
    }

    svtkIdType npts;
    const svtkIdType* pts;
    input->GetCellPoints(i, npts, pts);
    if (cellType == SVTK_VOXEL)
    {
      // Change point order: voxels and hexahedron don't have same connectivity.
      ids[0] = pts[0];
      ids[1] = pts[1];
      ids[2] = pts[3];
      ids[3] = pts[2];
      ids[4] = pts[4];
      ids[5] = pts[5];
      ids[6] = pts[7];
      ids[7] = pts[6];
      cells->ReplaceCellAtId(cellId, 8, ids);
    }
    else
    {
      cells->ReplaceCellAtId(cellId, 8, pts);
    }
    output->GetCellData()->CopyData(input->GetCellData(), i, cellId);
    if (expectedCells != nbCells)
    {
      // Unblank after copying the cell data to ensure it is not overwrited
      output->UnBlankCell(cellId);
    }
  }

  output->CheckAndReorderFaces();
  output->ComputeFacesConnectivityFlagsArray();
  return 1;
}

//----------------------------------------------------------------------------
int svtkUnstructuredGridToExplicitStructuredGrid::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}
