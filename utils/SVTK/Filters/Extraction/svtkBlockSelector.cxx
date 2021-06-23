/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBlockSelector.h"

#include "svtkArrayDispatch.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSetAttributes.h"
#include "svtkObjectFactory.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkUniformGridAMRDataIterator.h"

#include <set>

class svtkBlockSelector::svtkInternals
{
public:
  // This functor is only needed for svtkArrayDispatch to correctly fill it up.
  // Otherwise, it would simply be a set.
  class CompositeIdsT : public std::set<unsigned int>
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      using T = svtk::GetAPIType<ArrayType>;
      const auto range = svtk::DataArrayValueRange<1>(array);
      std::for_each(range.cbegin(), range.cend(),
        [&](const T val) { this->insert(static_cast<unsigned int>(val)); });
    }
  };

  // This functor is only needed for svtkArrayDispatch to correctly fill it up.
  // otherwise, it'd simply be a set.
  class AMRIdsT : public std::set<std::pair<unsigned int, unsigned int> >
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      const auto tuples = svtk::DataArrayTupleRange<2>(array);
      for (const auto tuple : tuples)
      {
        this->insert(
          std::make_pair(static_cast<unsigned int>(tuple[0]), static_cast<unsigned int>(tuple[1])));
      }
    }
  };

  CompositeIdsT CompositeIds;
  AMRIdsT AMRIds;
};

svtkStandardNewMacro(svtkBlockSelector);

//----------------------------------------------------------------------------
svtkBlockSelector::svtkBlockSelector()
{
  this->Internals = new svtkInternals;
}

//----------------------------------------------------------------------------
svtkBlockSelector::~svtkBlockSelector()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void svtkBlockSelector::Initialize(svtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  assert(this->Node->GetContentType() == svtkSelectionNode::BLOCKS);
  svtkDataArray* selectionList = svtkDataArray::SafeDownCast(this->Node->GetSelectionList());
  if (selectionList->GetNumberOfComponents() == 2)
  {
    if (!svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>::Execute(
          selectionList, this->Internals->AMRIds))
    {
      svtkGenericWarningMacro("SelectionList of unexpected type!");
    }
  }
  else if (selectionList->GetNumberOfComponents() == 1)
  {
    if (!svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>::Execute(
          selectionList, this->Internals->CompositeIds))
    {
      svtkGenericWarningMacro("SelectionList of unexpected type!");
    }
  }
}

//----------------------------------------------------------------------------
bool svtkBlockSelector::ComputeSelectedElements(
  svtkDataObject* svtkNotUsed(input), svtkSignedCharArray* insidednessArray)
{
  insidednessArray->FillValue(1);
  return true;
}

//----------------------------------------------------------------------------
svtkSelector::SelectionMode svtkBlockSelector::GetAMRBlockSelection(
  unsigned int level, unsigned int index)
{
  auto& internals = (*this->Internals);
  if (internals.AMRIds.find(std::make_pair(level, index)) != internals.AMRIds.end())
  {
    return INCLUDE;
  }
  else
  {
    return INHERIT;
  }
}

//----------------------------------------------------------------------------
svtkSelector::SelectionMode svtkBlockSelector::GetBlockSelection(unsigned int compositeIndex)
{
  auto& internals = (*this->Internals);
  if (internals.CompositeIds.find(compositeIndex) != internals.CompositeIds.end())
  {
    return INCLUDE;
  }
  else
  {
    return compositeIndex == 0 ? EXCLUDE : INHERIT;
  }
}

//----------------------------------------------------------------------------
void svtkBlockSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
