/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRToMultiBlockFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMRToMultiBlockFilter.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkUniformGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkAMRToMultiBlockFilter);

//------------------------------------------------------------------------------
svtkAMRToMultiBlockFilter::svtkAMRToMultiBlockFilter()
{
  this->Controller = svtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
svtkAMRToMultiBlockFilter::~svtkAMRToMultiBlockFilter() = default;

//------------------------------------------------------------------------------
void svtkAMRToMultiBlockFilter::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
}

//------------------------------------------------------------------------------
int svtkAMRToMultiBlockFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRToMultiBlockFilter::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void svtkAMRToMultiBlockFilter::CopyAMRToMultiBlock(
  svtkOverlappingAMR* amr, svtkMultiBlockDataSet* mbds)
{
  assert("pre: input AMR dataset is nullptr" && (amr != nullptr));
  assert("pre: output multi-block dataset is nullptr" && (mbds != nullptr));

  mbds->SetNumberOfBlocks(amr->GetTotalNumberOfBlocks());
  unsigned int blockIdx = 0;
  unsigned int levelIdx = 0;
  for (; levelIdx < amr->GetNumberOfLevels(); ++levelIdx)
  {
    unsigned int dataIdx = 0;
    for (; dataIdx < amr->GetNumberOfDataSets(levelIdx); ++dataIdx)
    {
      svtkUniformGrid* grid = amr->GetDataSet(levelIdx, dataIdx);
      if (grid != nullptr)
      {
        svtkUniformGrid* gridCopy = svtkUniformGrid::New();
        gridCopy->ShallowCopy(grid);
        mbds->SetBlock(blockIdx, gridCopy);
      }
      else
      {
        mbds->SetBlock(blockIdx, nullptr);
      }
      ++blockIdx;
    } // END for all data
  }   // END for all levels
}

//------------------------------------------------------------------------------
int svtkAMRToMultiBlockFilter::RequestData(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));
  svtkOverlappingAMR* amrds =
    svtkOverlappingAMR::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: input data-structure is nullptr" && (amrds != nullptr));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output Co information is nullptr" && (output != nullptr));
  svtkMultiBlockDataSet* mbds =
    svtkMultiBlockDataSet::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: output multi-block dataset is nullptr" && (mbds != nullptr));

  // STEP 2: Copy AMR data to multi-block
  this->CopyAMRToMultiBlock(amrds, mbds);

  return 1;
}
