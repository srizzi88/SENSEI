/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRDataSetCache.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "svtkAMRDataSetCache.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTimerLog.h"
#include "svtkUniformGrid.h"
#include <cassert>

svtkStandardNewMacro(svtkAMRDataSetCache);

svtkAMRDataSetCache::svtkAMRDataSetCache() = default;

//------------------------------------------------------------------------------
svtkAMRDataSetCache::~svtkAMRDataSetCache()
{
  AMRCacheType::iterator iter = this->Cache.begin();
  for (; iter != this->Cache.end(); ++iter)
  {
    if (iter->second != nullptr)
    {
      iter->second->Delete();
    }
  }
}

//------------------------------------------------------------------------------
void svtkAMRDataSetCache::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkAMRDataSetCache::InsertAMRBlock(int compositeIdx, svtkUniformGrid* amrGrid)
{
  assert("pre: AMR block is nullptr" && (amrGrid != nullptr));

  svtkTimerLog::MarkStartEvent("AMRCache::InsertBlock");
  if (!this->HasAMRBlock(compositeIdx))
  {
    this->Cache[compositeIdx] = amrGrid;
  }
  svtkTimerLog::MarkEndEvent("AMRCache::InsertBlock");
}

//------------------------------------------------------------------------------
void svtkAMRDataSetCache::InsertAMRBlockPointData(int compositeIdx, svtkDataArray* dataArray)
{
  assert("pre: AMR array is nullptr" && (dataArray != nullptr));
  assert("pre: AMR block is cached" && (this->HasAMRBlock(compositeIdx)));

  svtkTimerLog::MarkStartEvent("AMRCache::InsertAMRBlockPointData");

  svtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
  assert("pre: AMR block should not be nullptr" && (amrBlock != nullptr));

  svtkPointData* PD = amrBlock->GetPointData();
  assert("pre: PointData should not be nullptr" && (PD != nullptr));

  if (!PD->HasArray(dataArray->GetName()))
  {
    PD->AddArray(dataArray);
  }

  svtkTimerLog::MarkEndEvent("AMRCache::InsertAMRBlockPointData");
}

//------------------------------------------------------------------------------
void svtkAMRDataSetCache::InsertAMRBlockCellData(int compositeIdx, svtkDataArray* dataArray)
{
  assert("pre: AMR array is nullptr" && (dataArray != nullptr));
  assert("pre: AMR block is cached" && (this->HasAMRBlock(compositeIdx)));

  svtkTimerLog::MarkStartEvent("AMRCache::InsertAMRBlockCellData");

  svtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
  assert("pre: AMR block should not be nullptr" && (this->HasAMRBlock(compositeIdx)));

  svtkCellData* CD = amrBlock->GetCellData();
  assert("pre: CellData should not be nullptr" && (CD != nullptr));

  if (!CD->HasArray(dataArray->GetName()))
  {
    CD->AddArray(dataArray);
  }

  svtkTimerLog::MarkEndEvent("AMRCache::InsertAMRBlockCellData");
}

//------------------------------------------------------------------------------
svtkDataArray* svtkAMRDataSetCache::GetAMRBlockCellData(int compositeIdx, const char* dataName)
{
  if (this->HasAMRBlockCellData(compositeIdx, dataName))
  {
    svtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
    assert("pre: AMR block should not be nullptr" && (this->HasAMRBlock(compositeIdx)));

    svtkCellData* CD = amrBlock->GetCellData();
    assert("pre: CellData should not be nullptr" && (CD != nullptr));

    if (CD->HasArray(dataName))
    {
      return CD->GetArray(dataName);
    }
    else
    {
      return nullptr;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataArray* svtkAMRDataSetCache::GetAMRBlockPointData(int compositeIdx, const char* dataName)
{

  if (this->HasAMRBlockPointData(compositeIdx, dataName))
  {
    svtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
    assert("pre: AMR block should not be nullptr" && (amrBlock != nullptr));

    svtkPointData* PD = amrBlock->GetPointData();
    assert("pre: PointData should not be nullptr" && (PD != nullptr));

    if (PD->HasArray(dataName))
    {
      return PD->GetArray(dataName);
    }
    else
    {
      return nullptr;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkUniformGrid* svtkAMRDataSetCache::GetAMRBlock(const int compositeIdx)
{
  if (this->HasAMRBlock(compositeIdx))
  {
    return this->Cache[compositeIdx];
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool svtkAMRDataSetCache::HasAMRBlockCellData(int compositeIdx, const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));

  if (this->HasAMRBlock(compositeIdx))
  {
    svtkUniformGrid* gridPtr = this->GetAMRBlock(compositeIdx);
    assert("pre: cachedk block is nullptr!" && (gridPtr != nullptr));

    svtkCellData* CD = gridPtr->GetCellData();
    assert("pre: cell data is nullptr" && (CD != nullptr));

    if (CD->HasArray(name))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkAMRDataSetCache::HasAMRBlockPointData(int compositeIdx, const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));

  if (this->HasAMRBlock(compositeIdx))
  {
    svtkUniformGrid* gridPtr = this->GetAMRBlock(compositeIdx);
    assert("pre: cachedk block is nullptr!" && (gridPtr != nullptr));

    svtkPointData* PD = gridPtr->GetPointData();
    assert("pre: point data is nullptr" && (PD != nullptr));

    if (PD->HasArray(name))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool svtkAMRDataSetCache::HasAMRBlock(int compositeIdx)
{
  svtkTimerLog::MarkStartEvent("AMRCache::CheckIfBlockExists");

  if (this->Cache.empty())
  {
    svtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
    return false;
  }

  if (this->Cache.find(compositeIdx) != this->Cache.end())
  {
    svtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
    return true;
  }

  svtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
  return false;
}
