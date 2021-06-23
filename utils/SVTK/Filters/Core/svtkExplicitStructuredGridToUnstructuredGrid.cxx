/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridToUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExplicitStructuredGridToUnstructuredGrid.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataSetAttributes.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExplicitStructuredGridToUnstructuredGrid);

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridToUnstructuredGrid::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve input and output
  svtkExplicitStructuredGrid* input = svtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::GetData(outputVector, 0);

  // Copy field data
  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  // Copy input point data to output
  svtkDataSetAttributes* inPointData = static_cast<svtkDataSetAttributes*>(input->GetPointData());
  svtkDataSetAttributes* outPointData = static_cast<svtkDataSetAttributes*>(output->GetPointData());
  if (outPointData && inPointData)
  {
    outPointData->DeepCopy(inPointData);
  }

  output->SetPoints(input->GetPoints());

  // Initialize output cell data
  svtkDataSetAttributes* inCellData = static_cast<svtkDataSetAttributes*>(input->GetCellData());
  svtkDataSetAttributes* outCellData = static_cast<svtkDataSetAttributes*>(output->GetCellData());
  outCellData->CopyAllocate(inCellData);

  svtkIdType nbCells = input->GetNumberOfCells();

  // CellArray which links the new cells ids with the old ones
  svtkNew<svtkIdTypeArray> originalCellIds;
  originalCellIds->SetName("svtkOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->Allocate(nbCells);

  svtkNew<svtkIntArray> iArray;
  iArray->SetName("BLOCK_I");
  iArray->SetNumberOfComponents(1);
  iArray->Allocate(nbCells);

  svtkNew<svtkIntArray> jArray;
  jArray->SetName("BLOCK_J");
  jArray->SetNumberOfComponents(1);
  jArray->Allocate(nbCells);

  svtkNew<svtkIntArray> kArray;
  kArray->SetName("BLOCK_K");
  kArray->SetNumberOfComponents(1);
  kArray->Allocate(nbCells);

  svtkNew<svtkCellArray> cells;
  cells->AllocateEstimate(nbCells, 8);
  int i, j, k;
  for (svtkIdType cellId = 0; cellId < nbCells; cellId++)
  {
    if (input->IsCellVisible(cellId))
    {
      svtkNew<svtkIdList> ptIds;
      input->GetCellPoints(cellId, ptIds.GetPointer());
      svtkIdType newCellId = cells->InsertNextCell(ptIds.GetPointer());
      outCellData->CopyData(inCellData, cellId, newCellId);
      originalCellIds->InsertValue(newCellId, cellId);
      input->ComputeCellStructuredCoords(cellId, i, j, k);
      iArray->InsertValue(newCellId, i);
      jArray->InsertValue(newCellId, j);
      kArray->InsertValue(newCellId, k);
    }
  }
  originalCellIds->Squeeze();
  iArray->Squeeze();
  jArray->Squeeze();
  kArray->Squeeze();
  output->SetCells(SVTK_HEXAHEDRON, cells.GetPointer());
  outCellData->AddArray(originalCellIds.GetPointer());
  outCellData->AddArray(iArray.GetPointer());
  outCellData->AddArray(jArray.GetPointer());
  outCellData->AddArray(kArray.GetPointer());

  this->UpdateProgress(1.);

  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridToUnstructuredGrid::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkExplicitStructuredGrid");
  return 1;
}
