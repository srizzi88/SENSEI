/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToLinesFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMoleculeToLinesFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkMolecule.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkMoleculeToLinesFilter);

//----------------------------------------------------------------------------
int svtkMoleculeToLinesFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMolecule* input = svtkMolecule::SafeDownCast(svtkDataObject::GetData(inputVector[0]));
  svtkPolyData* output = svtkPolyData::SafeDownCast(svtkDataObject::GetData(outputVector));

  svtkNew<svtkCellArray> bonds;
  // 2 point ids + 1 SVTKCellType = 3 values per bonds
  bonds->AllocateEstimate(input->GetNumberOfBonds(), 2);

  for (svtkIdType bondInd = 0; bondInd < input->GetNumberOfBonds(); ++bondInd)
  {
    svtkBond bond = input->GetBond(bondInd);
    svtkIdType ids[2] = { bond.GetBeginAtomId(), bond.GetEndAtomId() };
    bonds->InsertNextCell(2, ids);
  }

  output->SetPoints(input->GetAtomicPositionArray());
  output->SetLines(bonds);
  output->GetPointData()->DeepCopy(input->GetAtomData());
  output->GetCellData()->DeepCopy(input->GetBondData());

  return 1;
}
