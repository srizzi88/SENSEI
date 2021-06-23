/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRFlashReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMRFlashReader.h"
#include "svtkAMRBox.h"
#include "svtkByteSwap.h"
#include "svtkDataArraySelection.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkUniformGrid.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkShortArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedShortArray.h"

#include <cassert>
#include <cfloat>
#include <map>
#include <vector>
#define H5_USE_16_API
#include "svtk_hdf5.h"

#include "svtkAMRFlashReaderInternal.h"

svtkStandardNewMacro(svtkAMRFlashReader);

//------------------------------------------------------------------------------
svtkAMRFlashReader::svtkAMRFlashReader()
{
  this->IsReady = false;
  this->Internal = new svtkFlashReaderInternal;
  this->Initialize();
}

//------------------------------------------------------------------------------
svtkAMRFlashReader::~svtkAMRFlashReader()
{
  delete[] this->FileName;
  this->FileName = nullptr;

  delete this->Internal;
  this->Internal = nullptr;
}

//-----------------------------------------------------------------------------
void svtkAMRFlashReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkAMRFlashReader::SetFileName(const char* fileName)
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));

  if (fileName && strcmp(fileName, "") &&
    ((this->FileName == nullptr) || strcmp(fileName, this->FileName)))
  {
    if (this->FileName)
    {
      delete[] this->FileName;
      this->FileName = nullptr;
      this->Internal->SetFileName(nullptr);
    }

    this->FileName = new char[strlen(fileName) + 1];
    strcpy(this->FileName, fileName);
    this->FileName[strlen(fileName)] = '\0';

    this->IsReady = true;
    this->Internal->SetFileName(this->FileName);
    this->LoadedMetaData = false;

    this->SetUpDataArraySelections();
    this->InitializeArraySelections();
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkAMRFlashReader::ReadMetaData()
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  this->Internal->ReadMetaData();
}

//-----------------------------------------------------------------------------
int svtkAMRFlashReader::GetBlockLevel(const int blockIdx)
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return (-1);
  }

  this->Internal->ReadMetaData();
  if (blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks)
  {
    svtkErrorMacro("Block Index (" << blockIdx << ") is out-of-bounds!");
    return (-1);
  }
  return (this->Internal->Blocks[blockIdx].Level - 1);
}

//-----------------------------------------------------------------------------
int svtkAMRFlashReader::GetNumberOfBlocks()
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return 0;
  }

  this->Internal->ReadMetaData();
  return (this->Internal->NumberOfBlocks);
}

//-----------------------------------------------------------------------------
int svtkAMRFlashReader::GetNumberOfLevels()
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  if (!this->IsReady)
  {
    return 0;
  }

  this->Internal->ReadMetaData();
  return (this->Internal->NumberOfLevels);
}

void svtkAMRFlashReader::ComputeStats(
  svtkFlashReaderInternal* internal, std::vector<int>& numBlocks, double min[3])
{
  min[0] = min[1] = min[2] = DBL_MAX;
  numBlocks.resize(this->Internal->NumberOfLevels, 0);

  for (int i = 0; i < internal->NumberOfBlocks; ++i)
  {
    Block& theBlock = internal->Blocks[i];
    double* gridMin = theBlock.MinBounds;
    if (gridMin[0] < min[0])
    {
      min[0] = gridMin[0];
    }
    if (gridMin[1] < min[1])
    {
      min[1] = gridMin[1];
    }
    if (gridMin[2] < min[2])
    {
      min[2] = gridMin[2];
    }
    int level = theBlock.Level - 1;
    numBlocks[level]++;
  }
}

//-----------------------------------------------------------------------------
int svtkAMRFlashReader::FillMetaData()
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  assert("pre: metadata object is nullptr" && (this->Metadata != nullptr));

  this->Internal->ReadMetaData();

  double origin[3];
  std::vector<int> blocksPerLevel;
  this->ComputeStats(this->Internal, blocksPerLevel, origin);

  this->Metadata->Initialize(static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);
  this->Metadata->SetGridDescription(SVTK_XYZ_GRID);
  this->Metadata->SetOrigin(origin);

  std::vector<int> b2level;
  b2level.resize(this->Internal->NumberOfLevels + 1, 0);

  for (int i = 0; i < this->Internal->NumberOfBlocks; ++i)
  {
    Block& theBlock = this->Internal->Blocks[i];

    // Start numbering levels from 0!
    int level = this->Internal->Blocks[i].Level - 1;
    int id = b2level[level];
    int internalIdx = i;
    int* dims = this->Internal->BlockGridDimensions;

    // compute spacing
    double spacing[3];
    for (int d = 0; d < 3; ++d)
    {
      spacing[d] =
        (dims[d] > 1) ? (theBlock.MaxBounds[d] - theBlock.MinBounds[d]) / (dims[d] - 1.0) : 1.0;
    }

    // compute AMRBox
    svtkAMRBox box(theBlock.MinBounds, dims, spacing, origin, SVTK_XYZ_GRID);

    this->Metadata->SetSpacing(level, spacing);
    this->Metadata->SetAMRBox(level, id, box);
    this->Metadata->SetAMRBlockSourceIndex(level, id, internalIdx);

    b2level[level]++;
  } // END for all blocks

  return (1);
}

//-----------------------------------------------------------------------------
svtkUniformGrid* svtkAMRFlashReader::GetAMRGrid(const int blockIdx)
{
  if (!this->IsReady)
  {
    return nullptr;
  }

  double blockMin[3];
  double blockMax[3];
  double spacings[3];

  for (int i = 0; i < 3; ++i)
  {
    blockMin[i] = this->Internal->Blocks[blockIdx].MinBounds[i];
    blockMax[i] = this->Internal->Blocks[blockIdx].MaxBounds[i];
    spacings[i] = (this->Internal->BlockGridDimensions[i] > 1)
      ? (blockMax[i] - blockMin[i]) / (this->Internal->BlockGridDimensions[i] - 1.0)
      : 1.0;
  }

  svtkUniformGrid* ug = svtkUniformGrid::New();
  ug->SetDimensions(this->Internal->BlockGridDimensions);
  ug->SetOrigin(blockMin[0], blockMin[1], blockMin[2]);
  ug->SetSpacing(spacings);
  return (ug);
}

//-----------------------------------------------------------------------------
void svtkAMRFlashReader::GetAMRGridData(const int blockIdx, svtkUniformGrid* block, const char* field)
{
  assert("pre: AMR block is nullptr" && (block != nullptr));
  this->Internal->GetBlockAttribute(field, blockIdx, block);
}

//-----------------------------------------------------------------------------
void svtkAMRFlashReader::SetUpDataArraySelections()
{
  assert("pre: Internal Flash Reader is nullptr" && (this->Internal != nullptr));
  this->Internal->ReadMetaData();

  int numAttrs = static_cast<int>(this->Internal->AttributeNames.size());
  for (int i = 0; i < numAttrs; ++i)
  {
    this->CellDataArraySelection->AddArray(this->Internal->AttributeNames[i].c_str());
  }
}
