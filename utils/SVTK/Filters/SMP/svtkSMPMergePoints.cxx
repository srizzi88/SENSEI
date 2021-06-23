/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPMergePoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "svtkSMPMergePoints.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkSMPMergePoints);

//------------------------------------------------------------------------------
svtkSMPMergePoints::svtkSMPMergePoints() = default;

//------------------------------------------------------------------------------
svtkSMPMergePoints::~svtkSMPMergePoints() = default;

//------------------------------------------------------------------------------
void svtkSMPMergePoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkSMPMergePoints::InitializeMerge()
{
  this->AtomicInsertionId = this->InsertionPointId;
}

//------------------------------------------------------------------------------
void svtkSMPMergePoints::Merge(svtkSMPMergePoints* locator, svtkIdType idx, svtkPointData* outPd,
  svtkPointData* ptData, svtkIdList* idList)
{
  if (!locator->HashTable[idx])
  {
    return;
  }

  svtkIdList *bucket, *oldIdToMerge;
  float* floatOldDataArray = nullptr;

  if (!(bucket = this->HashTable[idx]))
  {
    this->HashTable[idx] = bucket = svtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket / 2, this->NumberOfPointsPerBucket / 3);
    oldIdToMerge = locator->HashTable[idx];
    oldIdToMerge->Register(this);
    if (this->Points->GetData()->GetDataType() == SVTK_FLOAT)
    {
      floatOldDataArray = static_cast<svtkFloatArray*>(locator->Points->GetData())->GetPointer(0);
    }
  }
  else
  {
    oldIdToMerge = svtkIdList::New();

    svtkIdType nbOfIds = bucket->GetNumberOfIds();
    svtkIdType nbOfOldIds = locator->HashTable[idx]->GetNumberOfIds();
    oldIdToMerge->Allocate(nbOfOldIds);

    svtkDataArray* dataArray = this->Points->GetData();
    svtkDataArray* oldDataArray = locator->Points->GetData();
    svtkIdType* idArray = bucket->GetPointer(0);
    svtkIdType* idOldArray = locator->HashTable[idx]->GetPointer(0);

    bool found;

    if (dataArray->GetDataType() == SVTK_FLOAT)
    {
      float* floatDataArray = static_cast<svtkFloatArray*>(dataArray)->GetPointer(0);
      floatOldDataArray = static_cast<svtkFloatArray*>(oldDataArray)->GetPointer(0);

      for (svtkIdType oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx)
      {
        found = false;
        svtkIdType oldId = idOldArray[oldIdIdx];
        float* x = floatOldDataArray + 3 * oldId;
        float* pt;
        for (svtkIdType i = 0; i < nbOfIds; ++i)
        {
          svtkIdType existingId = idArray[i];
          pt = floatDataArray + 3 * existingId;
          if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId(oldId, existingId);
            break;
          }
        }
        if (!found)
        {
          oldIdToMerge->InsertNextId(oldId);
        }
      }
    }
    else
    {
      for (svtkIdType oldIdIdx = 0; oldIdIdx < nbOfOldIds; ++oldIdIdx)
      {
        found = false;
        svtkIdType oldId = idOldArray[oldIdIdx];
        double* x = oldDataArray->GetTuple(oldId);
        double* pt;
        for (svtkIdType i = 0; i < nbOfIds; ++i)
        {
          svtkIdType existingId = idArray[i];
          pt = dataArray->GetTuple(existingId);
          if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
          {
            // point is already in the list, return 0 and set the id parameter
            found = true;
            idList->SetId(oldId, existingId);
            break;
          }
        }
        if (!found)
        {
          oldIdToMerge->InsertNextId(oldId);
        }
      }
    }
  }

  // points have to be added
  svtkIdType numberOfInsertions = oldIdToMerge->GetNumberOfIds();
  svtkIdType firstId = this->AtomicInsertionId;
  this->AtomicInsertionId += numberOfInsertions;
  bucket->Resize(bucket->GetNumberOfIds() + numberOfInsertions);
  for (svtkIdType i = 0; i < numberOfInsertions; ++i)
  {
    svtkIdType newId = firstId + i, oldId = oldIdToMerge->GetId(i);
    idList->SetId(oldId, newId);
    bucket->InsertNextId(newId);
    if (floatOldDataArray)
    {
      const float* pt = floatOldDataArray + 3 * oldId;
      this->Points->SetPoint(newId, pt);
    }
    else
    {
      this->Points->SetPoint(newId, locator->Points->GetPoint(oldId));
    }
    outPd->SetTuple(newId, oldId, ptData);
  }
  oldIdToMerge->UnRegister(this);
}

//------------------------------------------------------------------------------
void svtkSMPMergePoints::FixSizeOfPointArray()
{
  this->Points->SetNumberOfPoints(this->AtomicInsertionId);
}
