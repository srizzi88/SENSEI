/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractDataSets.h"

#include "svtkCellData.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"

#include <cassert>
#include <set>

class svtkExtractDataSets::svtkInternals
{
public:
  struct Node
  {
    unsigned int Level;
    unsigned int Index;

    bool operator()(const Node& n1, const Node& n2) const
    {
      if (n1.Level == n2.Level)
      {
        return (n1.Index < n2.Index);
      }
      return (n1.Level < n2.Level);
    }
  };

  typedef std::set<Node, Node> DatasetsType;
  DatasetsType Datasets;
};

svtkStandardNewMacro(svtkExtractDataSets);
//----------------------------------------------------------------------------
svtkExtractDataSets::svtkExtractDataSets()
{
  this->Internals = new svtkInternals();
}

//----------------------------------------------------------------------------
svtkExtractDataSets::~svtkExtractDataSets()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void svtkExtractDataSets::AddDataSet(unsigned int level, unsigned int idx)
{
  svtkInternals::Node node;
  node.Level = level;
  node.Index = idx;
  this->Internals->Datasets.insert(node);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractDataSets::ClearDataSetList()
{
  this->Internals->Datasets.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int svtkExtractDataSets::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkExtractDataSets::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkExtractDataSets::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr!" && (inInfo != nullptr));
  svtkUniformGridAMR* input =
    svtkUniformGridAMR::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: input dataset is nullptr!" && (input != nullptr));

  // STEP 1: Get output
  svtkInformation* info = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr!" && (info != nullptr));
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: output dataset is nullptr!" && (output != nullptr));

  // STEP 2: Initialize structure
  output->SetNumberOfBlocks(input->GetNumberOfLevels());
  unsigned int blk = 0;
  for (; blk < output->GetNumberOfBlocks(); ++blk)
  {
    svtkMultiPieceDataSet* mpds = svtkMultiPieceDataSet::New();
    //      mpds->SetNumberOfPieces( input->GetNumberOfDataSets( blk ) );
    output->SetBlock(blk, mpds);
    mpds->Delete();
  } // END for all blocks/levels

  // STEP 3: Loop over sected blocks
  svtkInternals::DatasetsType::iterator iter = this->Internals->Datasets.begin();
  for (; iter != this->Internals->Datasets.end(); ++iter)
  {
    svtkUniformGrid* inUG = input->GetDataSet(iter->Level, iter->Index);
    if (inUG)
    {
      svtkMultiPieceDataSet* mpds =
        svtkMultiPieceDataSet::SafeDownCast(output->GetBlock(iter->Level));
      assert("pre: mpds is nullptr!" && (mpds != nullptr));

      unsigned int out_index = mpds->GetNumberOfPieces();
      svtkUniformGrid* clone = inUG->NewInstance();
      clone->ShallowCopy(inUG);

      // Remove blanking from output datasets.
      clone->GetCellData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
      mpds->SetPiece(out_index, clone);
      clone->Delete();
    }
  } // END for all selected items

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractDataSets::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
