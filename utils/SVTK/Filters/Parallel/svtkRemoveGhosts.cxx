/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveGhosts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "svtkRemoveGhosts.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <vector>

svtkStandardNewMacro(svtkRemoveGhosts);

//-----------------------------------------------------------------------------
svtkRemoveGhosts::svtkRemoveGhosts() = default;

//-----------------------------------------------------------------------------
svtkRemoveGhosts::~svtkRemoveGhosts() = default;

//-----------------------------------------------------------------------------
void svtkRemoveGhosts::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int svtkRemoveGhosts::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkRemoveGhosts::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDebugMacro("RequestData");

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnsignedCharArray* ghostArray = svtkUnsignedCharArray::SafeDownCast(
    input->GetCellData()->GetArray(svtkDataSetAttributes::GhostArrayName()));
  if (ghostArray == nullptr)
  {
    // no ghost information so can just shallow copy input
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  if (ghostArray && ghostArray->GetValueRange()[1] == 0)
  {
    // we have ghost cell arrays but there are no ghost entities so we just need
    // to remove those arrays and can skip modifying the data set itself
    output->ShallowCopy(input);
    output->GetPointData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
    output->GetCellData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
    return 1;
  }

  output->DeepCopy(input);
  if (svtkUnstructuredGrid* ugOutput = svtkUnstructuredGrid::SafeDownCast(output))
  {
    ugOutput->RemoveGhostCells();
  }
  else if (svtkPolyData* pdOutput = svtkPolyData::SafeDownCast(output))
  {
    pdOutput->RemoveGhostCells();
  }
  output->GetCellData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
  output->GetPointData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
  return 1;
}

//----------------------------------------------------------------------------
int svtkRemoveGhosts::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}
