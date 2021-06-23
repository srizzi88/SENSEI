/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRBaseReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMRBaseReader.h"
#include "svtkAMRDataSetCache.h"
#include "svtkAMRInformation.h"
#include "svtkCallbackCommand.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataArray.h"
#include "svtkDataArraySelection.h"
#include "svtkDataObject.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkOverlappingAMR.h"
#include "svtkParallelAMRUtilities.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTimerLog.h"
#include "svtkUniformGrid.h"

#include <cassert>

svtkAMRBaseReader::svtkAMRBaseReader()
{
  this->LoadedMetaData = false;
  this->NumBlocksFromCache = 0;
  this->NumBlocksFromFile = 0;
  this->EnableCaching = 0;
  this->Cache = nullptr;
}

//------------------------------------------------------------------------------
svtkAMRBaseReader::~svtkAMRBaseReader()
{
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();

  if (this->Cache != nullptr)
  {
    this->Cache->Delete();
  }

  if (this->Metadata != nullptr)
  {
    this->Metadata->Delete();
  }
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::Initialize()
{
  svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::Initialize");

  this->SetNumberOfInputPorts(0);
  this->FileName = nullptr;
  this->MaxLevel = 0;
  this->Metadata = nullptr;
  this->Controller = svtkMultiProcessController::GetGlobalController();
  this->InitialRequest = true;
  this->Cache = svtkAMRDataSetCache::New();

  this->CellDataArraySelection = svtkDataArraySelection::New();
  this->PointDataArraySelection = svtkDataArraySelection::New();
  this->SelectionObserver = svtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&svtkAMRBaseReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
  this->PointDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);

  svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::Initialize");
}

//----------------------------------------------------------------------------
void svtkAMRBaseReader::SelectionModifiedCallback(svtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<svtkAMRBaseReader*>(clientdata)->Modified();
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::GetNumberOfPointArrays()
{
  return (this->PointDataArraySelection->GetNumberOfArrays());
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::GetNumberOfCellArrays()
{
  return (this->CellDataArraySelection->GetNumberOfArrays());
}

//------------------------------------------------------------------------------
const char* svtkAMRBaseReader::GetPointArrayName(int index)
{
  return (this->PointDataArraySelection->GetArrayName(index));
}

//------------------------------------------------------------------------------
const char* svtkAMRBaseReader::GetCellArrayName(int index)
{
  return (this->CellDataArraySelection->GetArrayName(index));
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::GetPointArrayStatus(const char* name)
{
  return (this->PointDataArraySelection->ArrayIsEnabled(name));
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::GetCellArrayStatus(const char* name)
{
  return (this->CellDataArraySelection->ArrayIsEnabled(name));
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::SetPointArrayStatus(const char* name, int status)
{

  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::GetBlockProcessId(const int blockIdx)
{
  // If this is reader instance is serial, return Process 0
  // as the Process ID for the corresponding block.
  if (!this->IsParallel())
  {
    return 0;
  }

  int N = this->Controller->GetNumberOfProcesses();
  return (blockIdx % N);
}

//------------------------------------------------------------------------------
bool svtkAMRBaseReader::IsBlockMine(const int blockIdx)
{
  // If this reader instance does not run in parallel, then,
  // all blocks are owned by this reader.
  if (!this->IsParallel())
  {
    return true;
  }

  int myRank = this->Controller->GetLocalProcessId();
  if (myRank == this->GetBlockProcessId(blockIdx))
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::InitializeArraySelections()
{
  if (this->InitialRequest)
  {
    this->PointDataArraySelection->DisableAllArrays();
    this->CellDataArraySelection->DisableAllArrays();
    this->InitialRequest = false;
  }
}

//------------------------------------------------------------------------------
bool svtkAMRBaseReader::IsParallel()
{
  if (this->Controller == nullptr)
  {
    return false;
  }

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::RequestInformation(
  svtkInformation* rqst, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->LoadedMetaData)
  {
    return (1);
  }

  this->Superclass::RequestInformation(rqst, inputVector, outputVector);

  if (this->Metadata == nullptr)
  {
    this->Metadata = svtkOverlappingAMR::New();
  }
  else
  {
    this->Metadata->Initialize();
  }
  this->FillMetaData();
  svtkInformation* info = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (info != nullptr));
  info->Set(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), this->Metadata);
  if (this->Metadata && this->Metadata->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
  {
    double dataTime = this->Metadata->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
    info->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &dataTime, 1);
  }

  svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::GenerateParentChildInformation");
  this->Metadata->GenerateParentChildInformation();
  svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::GenerateParentChildInformation");

  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // std::cout<<"Generate Meta Data: ";
  // for(int levelIdx=0 ; levelIdx < this->Metadata->GetNumberOfLevels(); ++levelIdx )
  //  {
  //  std::cout << " \tL(" << levelIdx << ") = " << this->Metadata->GetNumberOfDataSets( levelIdx )
  //  << " "; std::cout.flush(); } // END for levels
  // std::cout<<endl;
  this->LoadedMetaData = true;
  return 1;
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::SetupBlockRequest(svtkInformation* outInf)
{
  assert("pre: output information is nullptr" && (outInf != nullptr));

  if (outInf->Has(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
  {
    assert("Metadata should not be null" && (this->Metadata != nullptr));
    this->ReadMetaData();

    int size = outInf->Length(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    int* indices = outInf->Get(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());

    this->BlockMap.clear();
    this->BlockMap.resize(size);

    for (int i = 0; i < size; ++i)
    {
      this->BlockMap[i] = indices[i];
    }
  }
  else
  {
    this->ReadMetaData();

    this->BlockMap.clear();
    int maxLevel = this->MaxLevel < static_cast<int>(this->Metadata->GetNumberOfLevels()) - 1
      ? this->MaxLevel
      : this->Metadata->GetNumberOfLevels() - 1;
    for (int level = 0; level <= maxLevel; level++)
    {
      for (unsigned int id = 0; id < this->Metadata->GetNumberOfDataSets(level); id++)
      {
        int index = this->Metadata->GetCompositeIndex(static_cast<unsigned int>(level), id);
        this->BlockMap.push_back(index);
      }
    }
  }
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::GetAMRData(const int blockIdx, svtkUniformGrid* block, const char* fieldName)
{
  assert("pre: AMR block is nullptr" && (block != nullptr));
  assert("pre: field name is nullptr" && (fieldName != nullptr));

  // If caching is disabled load the data from file
  if (!this->IsCachingEnabled())
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridDataFromFile");
    this->GetAMRGridData(blockIdx, block, fieldName);
    svtkTimerLog::MarkEndEvent("GetAMRGridDataFromFile");
    return;
  }

  // Caching is enabled.
  // Check the cache to see if the data has already been read.
  // Otherwise, read it and cache it.
  if (this->Cache->HasAMRBlockCellData(blockIdx, fieldName))
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridDataFromCache");
    svtkDataArray* data = this->Cache->GetAMRBlockCellData(blockIdx, fieldName);
    assert("pre: cached data is nullptr!" && (data != nullptr));
    svtkTimerLog::MarkEndEvent("GetAMRGridDataFromCache");

    block->GetCellData()->AddArray(data);
  }
  else
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridDataFromFile");
    this->GetAMRGridData(blockIdx, block, fieldName);
    svtkTimerLog::MarkEndEvent("GetAMRGridDataFromFile");

    svtkTimerLog::MarkStartEvent("CacheAMRData");
    this->Cache->InsertAMRBlockCellData(blockIdx, block->GetCellData()->GetArray(fieldName));
    svtkTimerLog::MarkEndEvent("CacheAMRData");
  }
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::GetAMRPointData(
  const int blockIdx, svtkUniformGrid* block, const char* fieldName)
{
  assert("pre: AMR block is nullptr" && (block != nullptr));
  assert("pre: field name is nullptr" && (fieldName != nullptr));

  // If caching is disabled load the data from file
  if (!this->IsCachingEnabled())
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridPointDataFromFile");
    this->GetAMRGridPointData(blockIdx, block, fieldName);
    svtkTimerLog::MarkEndEvent("GetAMRGridPointDataFromFile");
    return;
  }

  // Caching is enabled.
  // Check the cache to see if the data has already been read.
  // Otherwise, read it and cache it.
  if (this->Cache->HasAMRBlockPointData(blockIdx, fieldName))
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridPointDataFromCache");
    svtkDataArray* data = this->Cache->GetAMRBlockPointData(blockIdx, fieldName);
    assert("pre: cached data is nullptr!" && (data != nullptr));
    svtkTimerLog::MarkEndEvent("GetAMRGridPointDataFromCache");

    block->GetPointData()->AddArray(data);
  }
  else
  {
    svtkTimerLog::MarkStartEvent("GetAMRGridPointDataFromFile");
    this->GetAMRGridPointData(blockIdx, block, fieldName);
    svtkTimerLog::MarkEndEvent("GetAMRGridPointDataFromFile");

    svtkTimerLog::MarkStartEvent("CacheAMRPointData");
    this->Cache->InsertAMRBlockPointData(blockIdx, block->GetPointData()->GetArray(fieldName));
    svtkTimerLog::MarkEndEvent("CacheAMRPointData");
  }
}

//------------------------------------------------------------------------------
svtkUniformGrid* svtkAMRBaseReader::GetAMRBlock(const int blockIdx)
{

  // If caching is disabled load the data from file
  if (!this->IsCachingEnabled())
  {
    ++this->NumBlocksFromFile;
    svtkTimerLog::MarkStartEvent("ReadAMRBlockFromFile");
    svtkUniformGrid* gridPtr = this->GetAMRGrid(blockIdx);
    svtkTimerLog::MarkEndEvent("ReadAMRBlockFromFile");
    assert("pre: grid pointer is nullptr" && (gridPtr != nullptr));
    return (gridPtr);
  }

  // Caching is enabled.
  // Check the cache to see if the block has already been read.
  // Otherwise, read it and cache it.
  if (this->Cache->HasAMRBlock(blockIdx))
  {
    ++this->NumBlocksFromCache;
    svtkTimerLog::MarkStartEvent("ReadAMRBlockFromCache");
    svtkUniformGrid* gridPtr = svtkUniformGrid::New();
    svtkUniformGrid* cachedGrid = this->Cache->GetAMRBlock(blockIdx);
    gridPtr->CopyStructure(cachedGrid);
    svtkTimerLog::MarkEndEvent("ReadAMRBlockFromCache");
    return (gridPtr);
  }
  else
  {
    ++this->NumBlocksFromFile;
    svtkTimerLog::MarkStartEvent("ReadAMRBlockFromFile");
    svtkUniformGrid* cachedGrid = svtkUniformGrid::New();
    svtkUniformGrid* gridPtr = this->GetAMRGrid(blockIdx);
    assert("pre: grid pointer is nullptr" && (gridPtr != nullptr));
    svtkTimerLog::MarkEndEvent("ReadAMRBlockFromFile");

    svtkTimerLog::MarkStartEvent("CacheAMRBlock");
    cachedGrid->CopyStructure(gridPtr);
    this->Cache->InsertAMRBlock(blockIdx, cachedGrid);
    svtkTimerLog::MarkEndEvent("CacheAMRBlock");

    return (gridPtr);
  }

  //  assert( "Code should never reach here!" && (false) );
  //  return nullptr;
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::LoadPointData(const int blockIdx, svtkUniformGrid* block)
{
  // Sanity check!
  assert("pre: AMR block should not be nullptr" && (block != nullptr));

  for (int i = 0; i < this->GetNumberOfPointArrays(); ++i)
  {
    if (this->GetPointArrayStatus(this->GetPointArrayName(i)))
    {
      this->GetAMRPointData(blockIdx, block, this->GetPointArrayName(i));
    }
  } // END for all point arrays
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::LoadCellData(const int blockIdx, svtkUniformGrid* block)
{
  // Sanity check!
  assert("pre: AMR block should not be nullptr" && (block != nullptr));

  for (int i = 0; i < this->GetNumberOfCellArrays(); ++i)
  {
    if (this->GetCellArrayStatus(this->GetCellArrayName(i)))
    {
      this->GetAMRData(blockIdx, block, this->GetCellArrayName(i));
    }
  } // END for all cell arrays
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::LoadRequestedBlocks(svtkOverlappingAMR* output)
{
  assert("pre: AMR data-structure is nullptr" && (output != nullptr));

  // Unlike AssignAndLoadBlocks, this code doesn't have to bother about
  // "distributing" blocks to load among processes when running in parallel.
  // Sinks should ensure that request appropriate blocks (similar to the way
  // requests for pieces or extents work).
  for (size_t block = 0; block < this->BlockMap.size(); ++block)
  {
    // FIXME: this piece of code is very similar to the block in
    // AssignAndLoadBlocks. We should consolidate the two.
    int blockIndex = this->BlockMap[block];
    int blockIdx = this->Metadata->GetAMRInfo()->GetAMRBlockSourceIndex(blockIndex);

    unsigned int metaLevel;
    unsigned int metaIdx;
    this->Metadata->GetAMRInfo()->ComputeIndexPair(blockIndex, metaLevel, metaIdx);
    unsigned int level = this->GetBlockLevel(blockIdx);
    assert(level == metaLevel);

    // STEP 0: Get the AMR block
    svtkTimerLog::MarkStartEvent("GetAMRBlock");
    svtkUniformGrid* amrBlock = this->GetAMRBlock(blockIdx);
    svtkTimerLog::MarkEndEvent("GetAMRBlock");
    assert("pre: AMR block is nullptr" && (amrBlock != nullptr));

    // STEP 2: Load any point-data
    svtkTimerLog::MarkStartEvent("svtkARMBaseReader::LoadPointData");
    this->LoadPointData(blockIdx, amrBlock);
    svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::LoadPointData");

    // STEP 3: Load any cell data
    svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::LoadCellData");
    this->LoadCellData(blockIdx, amrBlock);
    svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::LoadCellData");

    // STEP 4: Add dataset
    output->SetDataSet(level, metaIdx, amrBlock);
    amrBlock->FastDelete();
  } // END for all blocks
}

//------------------------------------------------------------------------------
void svtkAMRBaseReader::AssignAndLoadBlocks(svtkOverlappingAMR* output)
{
  assert("pre: AMR data-structure is nullptr" && (output != nullptr));

  // Initialize counter of the number of blocks at each level.
  // This counter is used to compute the block index w.r.t. the
  // hierarchical box data-structure. Note that then number of blocks
  // can change based on user constraints, e.g., the number of levels
  // visible.
  std::vector<int> idxcounter;
  idxcounter.resize(this->GetNumberOfLevels() + 1, 0);

  // Find the number of blocks to be processed. BlockMap.size()
  // has all the blocks that are to be processesed and may be
  // less than or equal to this->GetNumberOfBlocks(), i.e., the
  // total number of blocks.
  int numBlocks = static_cast<int>(this->BlockMap.size());
  for (int block = 0; block < numBlocks; ++block)
  {
    int blockIndex = this->BlockMap[block];
    int blockIdx = this->Metadata->GetAMRInfo()->GetAMRBlockSourceIndex(blockIndex);

    unsigned int metaLevel;
    unsigned int metaIdx;
    this->Metadata->GetAMRInfo()->ComputeIndexPair(blockIndex, metaLevel, metaIdx);
    unsigned int level = this->GetBlockLevel(blockIdx);
    assert(level == metaLevel);

    if (this->IsBlockMine(block))
    {
      // STEP 0: Get the AMR block
      svtkTimerLog::MarkStartEvent("GetAMRBlock");
      svtkUniformGrid* amrBlock = this->GetAMRBlock(blockIdx);
      svtkTimerLog::MarkEndEvent("GetAMRBlock");
      assert("pre: AMR block is nullptr" && (amrBlock != nullptr));

      // STEP 2: Load any point-data
      svtkTimerLog::MarkStartEvent("svtkARMBaseReader::LoadPointData");
      this->LoadPointData(blockIdx, amrBlock);
      svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::LoadPointData");

      // STEP 3: Load any cell data
      svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::LoadCellData");
      this->LoadCellData(blockIdx, amrBlock);
      svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::LoadCellData");

      // STEP 4: Add dataset
      output->SetDataSet(level, metaIdx, amrBlock);
      amrBlock->Delete();
    } // END if the block belongs to this process
    else
    {
      output->SetDataSet(level, metaIdx, nullptr);
    }
  } // END for all blocks
}

//------------------------------------------------------------------------------
int svtkAMRBaseReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::RqstData");
  this->NumBlocksFromCache = 0;
  this->NumBlocksFromFile = 0;

  svtkInformation* outInf = outputVector->GetInformationObject(0);
  svtkOverlappingAMR* output =
    svtkOverlappingAMR::SafeDownCast(outInf->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: output AMR dataset is nullptr" && (output != nullptr));

  output->SetAMRInfo(this->Metadata->GetAMRInfo());

  // Setup the block request
  svtkTimerLog::MarkStartEvent("svtkAMRBaseReader::SetupBlockRequest");
  this->SetupBlockRequest(outInf);
  svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::SetupBlockRequest");

  if (outInf->Has(svtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()))
  {
    this->LoadRequestedBlocks(output);

    // Is blanking information generated when only a subset of blocks is
    // requested? Tricky question, since we need the blanking information when
    // requesting a fixed set of blocks and when when requesting one block at a
    // time in streaming fashion.
  }
  else
  {
#ifdef DEBUGME
    cout << "load " << this->BlockMap.size() << " blocks" << endl;
#endif
    this->AssignAndLoadBlocks(output);

    svtkTimerLog::MarkStartEvent("AMR::Generate Blanking");
    svtkParallelAMRUtilities::BlankCells(output, this->Controller);
    svtkTimerLog::MarkEndEvent("AMR::Generate Blanking");
  }

  // If this instance of the reader is not parallel, block until all processes
  // read their blocks.
  if (this->IsParallel())
  {
    this->Controller->Barrier();
  }

  if (this->Metadata && this->Metadata->GetInformation()->Has(svtkDataObject::DATA_TIME_STEP()))
  {
    double dataTime = this->Metadata->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), dataTime);
  }

  outInf = nullptr;
  output = nullptr;

  svtkTimerLog::MarkEndEvent("svtkAMRBaseReader::RqstData");

  return 1;
}
