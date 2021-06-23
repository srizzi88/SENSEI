/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetToMoleculeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointSetToMoleculeFilter.h"

#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"

svtkStandardNewMacro(svtkPointSetToMoleculeFilter);

//----------------------------------------------------------------------------
svtkPointSetToMoleculeFilter::svtkPointSetToMoleculeFilter()
  : ConvertLinesIntoBonds(true)
{
  this->SetNumberOfInputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int svtkPointSetToMoleculeFilter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPointSetToMoleculeFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPointSet* input = svtkPointSet::SafeDownCast(svtkDataObject::GetData(inputVector[0]));
  svtkMolecule* output = svtkMolecule::SafeDownCast(svtkDataObject::GetData(outputVector));

  if (!input)
  {
    svtkErrorMacro(<< "No input provided.");
    return 0;
  }

  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (input->GetNumberOfPoints() > 0 && !inScalars)
  {
    svtkErrorMacro(<< "svtkPointSetToMoleculeFilter does not have atomic numbers as input.");
    return 0;
  }

  int res = output->Initialize(input->GetPoints(), inScalars, input->GetPointData());

  if (res != 0 && this->GetConvertLinesIntoBonds())
  {
    svtkNew<svtkIdList> inputBondsId;
    svtkNew<svtkIdList> outputBondsId;
    svtkSmartPointer<svtkCellIterator> iter =
      svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());
    // Get bond orders array. Use scalars as default.
    svtkDataArray* bondOrders = input->GetCellData()->HasArray(output->GetBondOrdersArrayName())
      ? input->GetCellData()->GetArray(output->GetBondOrdersArrayName())
      : input->GetCellData()->GetScalars();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
      if (iter->GetCellType() != SVTK_LINE)
      {
        continue;
      }
      svtkIdList* ptsId = iter->GetPointIds();
      unsigned short bondOrder = bondOrders ? bondOrders->GetTuple1(iter->GetCellId()) : 1;
      svtkBond bond = output->AppendBond(ptsId->GetId(0), ptsId->GetId(1), bondOrder);
      inputBondsId->InsertNextId(iter->GetCellId());
      outputBondsId->InsertNextId(bond.GetId());
    }

    output->GetBondData()->CopyAllocate(input->GetCellData());
    output->GetBondData()->CopyData(input->GetCellData(), inputBondsId, outputBondsId);
  }

  return res;
}
