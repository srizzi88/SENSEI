/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractLevel.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridAMR.h"

#include <set>
#include <vector>

class svtkExtractLevel::svtkSet : public std::set<unsigned int>
{
};

svtkStandardNewMacro(svtkExtractLevel);
//----------------------------------------------------------------------------
svtkExtractLevel::svtkExtractLevel()
{
  this->Levels = new svtkExtractLevel::svtkSet();
}

//----------------------------------------------------------------------------
svtkExtractLevel::~svtkExtractLevel()
{
  delete this->Levels;
}

//----------------------------------------------------------------------------
void svtkExtractLevel::AddLevel(unsigned int level)
{
  this->Levels->insert(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractLevel::RemoveLevel(unsigned int level)
{
  this->Levels->erase(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractLevel::RemoveAllLevels()
{
  this->Levels->clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int svtkExtractLevel::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkExtractLevel::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

int svtkExtractLevel::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // Check if metadata are passed downstream
  if (inInfo->Has(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    svtkOverlappingAMR* metadata = svtkOverlappingAMR::SafeDownCast(
      inInfo->Get(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    if (metadata)
    {
      // cout<<"Time dependent?
      // "<<inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION())<<endl;
      // std::cout<<"Receive Meta Data: ";
      // for(int levelIdx=0 ; levelIdx < metadata->GetNumberOfLevels(); ++levelIdx )
      //   {
      //   std::cout << " \tL(" << levelIdx << ") = "
      //             << metadata->GetNumberOfDataSets( levelIdx ) << " ";
      //   std::cout.flush();
      //   } // END for levels
      // std::cout<<endl;

      // Tell reader to load all requested blocks.
      inInfo->Set(svtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);

      // request the blocks
      std::vector<int> blocksToLoad;
      for (svtkExtractLevel::svtkSet::iterator iter = this->Levels->begin();
           iter != this->Levels->end(); ++iter)
      {
        unsigned int level = (*iter);
        for (unsigned int dataIdx = 0; dataIdx < metadata->GetNumberOfDataSets(level); ++dataIdx)
        {
          blocksToLoad.push_back(metadata->GetCompositeIndex(level, dataIdx));
        }
      }

      inInfo->Set(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &blocksToLoad[0],
        static_cast<int>(blocksToLoad.size()));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractLevel::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input object

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkUniformGridAMR* input =
    svtkUniformGridAMR::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (input == nullptr)
  {
    return (0);
  }

  // STEP 1: Get output object
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (output == nullptr)
  {
    return (0);
  }

  // STEP 2: Compute the total number of blocks to be loaded
  unsigned int numBlocksToLoad = 0;
  svtkExtractLevel::svtkSet::iterator iter;
  for (iter = this->Levels->begin(); iter != this->Levels->end(); ++iter)
  {
    unsigned int level = (*iter);
    numBlocksToLoad += input->GetNumberOfDataSets(level);
  } // END for all requested levels
  output->SetNumberOfBlocks(numBlocksToLoad);

  // STEP 3: Load the blocks at the selected levels
  if (numBlocksToLoad > 0)
  {
    iter = this->Levels->begin();
    unsigned int blockIdx = 0;
    for (; iter != this->Levels->end(); ++iter)
    {
      unsigned int level = (*iter);
      unsigned int dataIdx = 0;
      for (; dataIdx < input->GetNumberOfDataSets(level); ++dataIdx)
      {
        svtkUniformGrid* data = input->GetDataSet(level, dataIdx);
        if (data != nullptr)
        {
          svtkUniformGrid* copy = data->NewInstance();
          copy->ShallowCopy(data);
          output->SetBlock(blockIdx, copy);
          copy->Delete();
          ++blockIdx;
        } // END if data is not nullptr
      }   // END for all data at level l
    }     // END for all requested levels
  }       // END if numBlocksToLoad is greater than 0

  return (1);
}

//----------------------------------------------------------------------------
void svtkExtractLevel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
