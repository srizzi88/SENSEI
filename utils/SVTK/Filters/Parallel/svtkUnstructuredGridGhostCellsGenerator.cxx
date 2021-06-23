/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkUnstructuredGridGhostCellsGenerator.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

namespace
{
const char* UGGCG_GLOBAL_POINT_IDS = "GlobalNodeIds";
const char* UGGCG_GLOBAL_CELL_IDS = "GlobalCellIds";
}

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkUnstructuredGridGhostCellsGenerator);

//----------------------------------------------------------------------------
svtkUnstructuredGridGhostCellsGenerator::svtkUnstructuredGridGhostCellsGenerator()
{
  this->BuildIfRequired = true;
  this->MinimumNumberOfGhostLevels = 1;

  this->UseGlobalPointIds = true;
  this->GlobalPointIdsArrayName = nullptr;
  this->SetGlobalPointIdsArrayName(UGGCG_GLOBAL_POINT_IDS);

  this->HasGlobalCellIds = false;
  this->GlobalCellIdsArrayName = nullptr;
  this->SetGlobalCellIdsArrayName(UGGCG_GLOBAL_CELL_IDS);
}

//----------------------------------------------------------------------------
svtkUnstructuredGridGhostCellsGenerator::~svtkUnstructuredGridGhostCellsGenerator()
{
  this->SetGlobalPointIdsArrayName(nullptr);
  this->SetGlobalCellIdsArrayName(nullptr);
}

//-----------------------------------------------------------------------------
void svtkUnstructuredGridGhostCellsGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "UseGlobalPointIds: " << this->UseGlobalPointIds << endl;
  os << indent << "GlobalPointIdsArrayName: "
     << (this->GlobalPointIdsArrayName == nullptr ? "(nullptr)" : this->GlobalPointIdsArrayName)
     << endl;
  os << indent << "HasGlobalCellIds: " << HasGlobalCellIds << endl;
  os << indent << "GlobalCellIdsArrayName: "
     << (this->GlobalCellIdsArrayName == nullptr ? "(nullptr)" : this->GlobalCellIdsArrayName)
     << endl;
  os << indent << "BuildIfRequired: " << this->BuildIfRequired << endl;
  os << indent << "MinimumNumberOfGhostLevels: " << this->MinimumNumberOfGhostLevels << endl;
}

//--------------------------------------------------------------------------
int svtkUnstructuredGridGhostCellsGenerator::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  // we can't trust any ghost levels coming in so we notify all filters before
  // this that we don't need ghosts
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

//-----------------------------------------------------------------------------
int svtkUnstructuredGridGhostCellsGenerator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  svtkUnstructuredGridBase* input =
    svtkUnstructuredGridBase::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    svtkErrorMacro("No input data!");
    return 0;
  }

  output->ShallowCopy(input);
  return 1;
}
