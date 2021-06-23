/*=========================================================================

  Program:   ParaView
  Module:    svtkPExtractDataArraysOverTime.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPExtractDataArraysOverTime.h"

#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkMultiProcessStream.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"

#include <cassert>
#include <map>
#include <sstream>
#include <string>

namespace
{
svtkSmartPointer<svtkTable> svtkMergeTable(svtkTable* dest, svtkTable* src)
{
  if (dest == nullptr)
  {
    return src;
  }

  const auto numRows = dest->GetNumberOfRows();
  if (numRows != src->GetNumberOfRows())
  {
    return dest;
  }

  auto srcRowData = src->GetRowData();
  auto destRowData = dest->GetRowData();
  auto srcMask = svtkUnsignedCharArray::SafeDownCast(srcRowData->GetArray("svtkValidPointMask"));
  for (svtkIdType cc = 0; srcMask != nullptr && cc < numRows; ++cc)
  {
    if (srcMask->GetTypedComponent(cc, 0) == 0)
    {
      continue;
    }

    // Copy arrays from remote to current
    const int numArray = srcRowData->GetNumberOfArrays();
    for (int aidx = 0; aidx < numArray; aidx++)
    {
      auto srcArray = srcRowData->GetAbstractArray(aidx);
      if (const char* name = srcArray ? srcArray->GetName() : nullptr)
      {
        auto destArray = destRowData->GetAbstractArray(name);
        if (destArray == nullptr)
        {
          // add new array array if necessary
          destRowData->AddArray(destArray);
        }
        else
        {
          destArray->InsertTuple(cc, cc, srcArray);
        }
      }
    }
  }
  return dest;
}
}

svtkStandardNewMacro(svtkPExtractDataArraysOverTime);
svtkCxxSetObjectMacro(svtkPExtractDataArraysOverTime, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkPExtractDataArraysOverTime::svtkPExtractDataArraysOverTime()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPExtractDataArraysOverTime::~svtkPExtractDataArraysOverTime()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void svtkPExtractDataArraysOverTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//----------------------------------------------------------------------------
void svtkPExtractDataArraysOverTime::PostExecute(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::PostExecute(request, inputVector, outputVector);
  if (!this->Controller || this->Controller->GetNumberOfProcesses() < 2)
  {
    return;
  }

  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);
  assert(output);
  this->ReorganizeData(output);
}

//----------------------------------------------------------------------------
void svtkPExtractDataArraysOverTime::ReorganizeData(svtkMultiBlockDataSet* dataset)
{
  // 1. Send all blocks to 0.
  // 2. Rank 0 then reorganizes blocks. This is done as follows:
  //    i. If blocks form different ranks have same names, then we check if they
  //       are referring to the same global-id. If so, the tables are merged
  //       into one. If not, we the tables separately, with their names
  //       uniquified with rank number.
  // 3. Rank 0 send info about number blocks and their names to everyone
  // 4. Satellites, then, simply initialize their output to and make it match
  //    the structure reported by rank 0.

  const int myRank = this->Controller->GetLocalProcessId();
  const int numRanks = this->Controller->GetNumberOfProcesses();
  if (myRank != 0)
  {
    std::vector<svtkSmartPointer<svtkDataObject> > recvBuffer;
    this->Controller->Gather(dataset, recvBuffer, 0);

    svtkMultiProcessStream stream;
    this->Controller->Broadcast(stream, 0);

    dataset->Initialize();
    while (!stream.Empty())
    {
      std::string name;
      stream >> name;

      auto idx = dataset->GetNumberOfBlocks();
      dataset->SetBlock(idx, nullptr);
      dataset->GetMetaData(idx)->Set(svtkCompositeDataSet::NAME(), name.c_str());
    }
  }
  else
  {
    std::vector<svtkSmartPointer<svtkDataObject> > recvBuffer;
    this->Controller->Gather(dataset, recvBuffer, 0);

    assert(static_cast<int>(recvBuffer.size()) == numRanks);

    recvBuffer[myRank] = dataset;

    std::map<std::string, std::map<int, svtkSmartPointer<svtkTable> > > collection;
    for (int rank = 0; rank < numRanks; ++rank)
    {
      if (auto mb = svtkMultiBlockDataSet::SafeDownCast(recvBuffer[rank]))
      {
        for (unsigned int cc = 0, max = mb->GetNumberOfBlocks(); cc < max; ++cc)
        {
          const char* name = mb->GetMetaData(cc)->Get(svtkCompositeDataSet::NAME());
          svtkTable* subblock = svtkTable::SafeDownCast(mb->GetBlock(cc));
          if (name && subblock)
          {
            collection[name][rank] = subblock;
          }
        }
      }
    }

    svtkMultiProcessStream stream;
    svtkNew<svtkMultiBlockDataSet> mb;
    for (auto& item : collection)
    {
      const std::string& name = item.first;

      // as we using global ids, if so merge the tables.
      if (strncmp(name.c_str(), "gid=", 4) == 0)
      {
        svtkSmartPointer<svtkTable> mergedTable;
        for (auto& sitem : item.second)
        {
          mergedTable = svtkMergeTable(mergedTable, sitem.second);
        }

        auto idx = mb->GetNumberOfBlocks();
        mb->SetBlock(idx, mergedTable);
        mb->GetMetaData(idx)->Set(svtkCompositeDataSet::NAME(), name.c_str());
        stream << name;
      }
      else
      {
        // if not gids, then add each table separately with rank info.
        for (auto& sitem : item.second)
        {
          auto idx = mb->GetNumberOfBlocks();
          mb->SetBlock(idx, sitem.second);

          std::ostringstream str;
          str << name << " rank=" << sitem.first;
          mb->GetMetaData(idx)->Set(svtkCompositeDataSet::NAME(), str.str().c_str());
          stream << str.str();
        }
      }
    }

    this->Controller->Broadcast(stream, 0);
    dataset->ShallowCopy(mb);
  } // end rank 0
}
