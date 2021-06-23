/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDistributedDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkPDistributedDataFilter.h"

#include "svtkBSPCuts.h"
#include "svtkBox.h"
#include "svtkBoxClipDataSet.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkClipDataSet.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSetReader.h"
#include "svtkDataSetWriter.h"
#include "svtkExtractCells.h"
#include "svtkExtractUserDefinedPiece.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMergeCells.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPKdTree.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkSmartPointer.h"
#include "svtkSocketController.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTimerLog.h"
#include "svtkToolkits.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include "svtkMPIController.h"

#include <vector>

svtkStandardNewMacro(svtkPDistributedDataFilter);

#define TEMP_ELEMENT_ID_NAME "___D3___GlobalCellIds"
#define TEMP_INSIDE_BOX_FLAG "___D3___WHERE"
#define TEMP_NODE_ID_NAME "___D3___GlobalNodeIds"

#include <algorithm>
#include <map>
#include <set>

namespace
{
class TimeLog // Similar to svtkTimerLogScope, but can be disabled at runtime.
{
  const std::string Event;
  int Timing;
  bool Entry;

public:
  TimeLog(const char* event, int timing, bool entry = false)
    : Event(event ? event : "")
    , Timing(timing)
    , Entry(entry)
  {
    if (this->Timing)
    {
      if (this->Entry)
      {
        svtkTimerLog::SetMaxEntries(std::max(svtkTimerLog::GetMaxEntries(), 250));
        svtkTimerLog::ResetLog();
        svtkTimerLog::LoggingOn();
      }
      svtkTimerLog::MarkStartEvent(this->Event.c_str());
    }
  }

  ~TimeLog()
  {
    if (this->Timing)
    {
      svtkTimerLog::MarkEndEvent(this->Event.c_str());
      if (this->Entry)
      {
        svtkTimerLog::DumpLogWithIndentsAndPercentages(&std::cout);
        std::cout << std::endl;
        svtkTimerLog::ResetLog();
      }
    }
  }

  static void StartEvent(const char* event, int timing)
  {
    if (timing)
    {
      svtkTimerLog::MarkStartEvent(event);
    }
  }

  static void EndEvent(const char* event, int timing)
  {
    if (timing)
    {
      svtkTimerLog::MarkEndEvent(event);
    }
  }

private:
  // Explicit disable copy/assignment to prevent MSVC from complaining (C4512)
  TimeLog(const TimeLog&) = delete;
  TimeLog& operator=(const TimeLog&) = delete;
};
}

class svtkPDistributedDataFilterSTLCloak
{
public:
  std::map<int, int> IntMap;
  std::multimap<int, int> IntMultiMap;
};

namespace
{
void convertGhostLevelsToBitFields(svtkDataSetAttributes* dsa, unsigned int bit)
{
  svtkDataArray* da = dsa->GetArray(svtkDataSetAttributes::GhostArrayName());
  svtkUnsignedCharArray* uca = svtkArrayDownCast<svtkUnsignedCharArray>(da);
  unsigned char* ghosts = uca->GetPointer(0);
  for (svtkIdType i = 0; i < da->GetNumberOfTuples(); ++i)
  {
    if (ghosts[i] > 0)
    {
      ghosts[i] = bit;
    }
  }
}
};

//----------------------------------------------------------------------------
svtkPDistributedDataFilter::svtkPDistributedDataFilter() {}

//----------------------------------------------------------------------------
svtkPDistributedDataFilter::~svtkPDistributedDataFilter() {}

//----------------------------------------------------------------------------
svtkIdTypeArray* svtkPDistributedDataFilter::GetGlobalElementIdArray(svtkDataSet* set)
{
  svtkDataArray* da = set->GetCellData()->GetGlobalIds();
  return svtkArrayDownCast<svtkIdTypeArray>(da);
}

//----------------------------------------------------------------------------
svtkIdType* svtkPDistributedDataFilter::GetGlobalElementIds(svtkDataSet* set)
{
  svtkIdTypeArray* ia = GetGlobalElementIdArray(set);
  if (!ia)
  {
    return nullptr;
  }

  return ia->GetPointer(0);
}

//----------------------------------------------------------------------------
svtkIdTypeArray* svtkPDistributedDataFilter::GetGlobalNodeIdArray(svtkDataSet* set)
{
  svtkDataArray* da = set->GetPointData()->GetGlobalIds();
  return svtkArrayDownCast<svtkIdTypeArray>(da);
}

//----------------------------------------------------------------------------
svtkIdType* svtkPDistributedDataFilter::GetGlobalNodeIds(svtkDataSet* set)
{
  svtkIdTypeArray* ia = this->GetGlobalNodeIdArray(set);

  if (!ia)
  {
    return nullptr;
  }

  return ia->GetPointer(0);
}

//============================================================================
// Execute

//----------------------------------------------------------------------------
int svtkPDistributedDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  TimeLog timer("D3::RequestData", this->Timing, true);
  (void)timer;

  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->GhostLevel =
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  this->GhostLevel = std::max(this->GhostLevel, this->MinimumGhostLevel);

  // get the input and output
  svtkDataSet* inputDS = svtkDataSet::GetData(inputVector[0], 0);
  svtkUnstructuredGrid* outputUG = svtkUnstructuredGrid::GetData(outInfo);
  if (inputDS && outputUG)
  {
    return this->RequestDataInternal(inputDS, outputUG);
  }

  svtkCompositeDataSet* inputCD = svtkCompositeDataSet::GetData(inputVector[0], 0);
  svtkMultiBlockDataSet* outputMB = svtkMultiBlockDataSet::GetData(outputVector, 0);
  if (!inputCD || !outputMB)
  {
    svtkErrorMacro("Input must either be a composite dataset or a svtkDataSet.");
    return 0;
  }

  outputMB->CopyStructure(inputCD);

  TimeLog::StartEvent("Classify leaves", this->Timing);
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(inputCD->NewIterator());
  // We want to traverse over empty nodes as well. This ensures that this
  // algorithm will work correctly in parallel.
  iter->SkipEmptyNodesOff();

  // Collect information about datatypes all the processes have at all the leaf
  // nodes. Ideally all processes will either have the same type or an empty
  // dataset. This assumes that all processes have the same composite structure.
  std::vector<int> leafTypes;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* dObj = iter->GetCurrentDataObject();
    if (dObj)
    {
      leafTypes.push_back(dObj->GetDataObjectType());
    }
    else
    {
      leafTypes.push_back(-1);
    }
  }
  unsigned int numLeaves = static_cast<unsigned int>(leafTypes.size());

  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs > 1 && numLeaves > 0)
  {
    if (myId == 0)
    {
      for (int cc = 1; cc < numProcs; cc++)
      {
        std::vector<int> receivedTypes;
        receivedTypes.resize(numLeaves, -1);
        if (!this->Controller->Receive(&receivedTypes[0], numLeaves, cc, 1020202))
        {
          svtkErrorMacro("Communication error.");
          return 0;
        }
        for (unsigned int kk = 0; kk < numLeaves; kk++)
        {
          if (leafTypes[kk] == -1)
          {
            leafTypes[kk] = receivedTypes[kk];
          }
          if (receivedTypes[kk] != -1 && leafTypes[kk] != -1 && receivedTypes[kk] != leafTypes[kk])
          {
            svtkWarningMacro("Data type mismatch on processes.");
          }
        }
      }
      for (int kk = 1; kk < numProcs; kk++)
      {
        this->Controller->Send(&leafTypes[0], numLeaves, kk, 1020203);
      }
    }
    else
    {
      this->Controller->Send(&leafTypes[0], numLeaves, 0, 1020202);
      this->Controller->Receive(&leafTypes[0], numLeaves, 0, 1020203);
    }
  }
  TimeLog::EndEvent("Classify leaves", this->Timing);

  unsigned int cc = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), cc++)
  {
    svtkSmartPointer<svtkDataSet> ds;
    ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds == nullptr)
    {
      if (leafTypes[cc] == -1)
      {
        // This is an empty block on all processes, just skip it.
        continue;
      }

      ds.TakeReference(svtkDataSet::SafeDownCast(svtkDataObjectTypes::NewDataObject(leafTypes[cc])));
    }
    svtkSmartPointer<svtkUnstructuredGrid> ug = svtkSmartPointer<svtkUnstructuredGrid>::New();
    if (!this->RequestDataInternal(ds, ug))
    {
      return 0;
    }
    if (ug->GetNumberOfPoints() > 0)
    {
      outputMB->SetDataSet(iter, ug);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPDistributedDataFilter::RequestDataInternal(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  TimeLog timer("RequestDataInternal", this->Timing);
  (void)timer;

  this->NextProgressStep = 0;
  int progressSteps = 5 + this->GhostLevel;
  if (this->ClipCells)
  {
    progressSteps++;
  }

  this->ProgressIncrement = 1.0 / (double)progressSteps;

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Begin data redistribution");

  if (this->NumProcesses == 1)
  {
    this->SingleProcessExecute(input, output);
    this->UpdateProgress(1.0);
    return 1;
  }

  // This method requires an MPI controller.

  int aok = 0;

  if (svtkMPIController::SafeDownCast(this->Controller))
  {
    aok = 1;
  }

  if (!aok)
  {
    svtkErrorMacro(<< "svtkPDistributedDataFilter multiprocess requires MPI");
    return 1;
  }

  // Stage (0) - If any processes have 0 cell input data sets, then
  //   spread the input data sets around (quickly) before formal
  //   redistribution.

  int duplicateCells;
  svtkDataSet* splitInput = this->TestFixTooFewInputFiles(input, duplicateCells);

  if (splitInput == nullptr)
  {
    return 1; // Fewer cells than processes - can't divide input
  }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Compute spatial partitioning");

  // Stage (1) - use svtkPKdTree to...
  //   Create a load balanced spatial decomposition in parallel.
  //   Create a table assigning regions to processes.
  //
  // Note k-d tree will only be re-built if input or parameters
  // have changed on any of the processing nodes.

  int fail = this->PartitionDataAndAssignToProcesses(splitInput);

  if (fail)
  {
    if (splitInput != input)
    {
      splitInput->Delete();
    }
    svtkErrorMacro(<< "svtkPDistributedDataFilter::Execute k-d tree failure");
    return 1;
  }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Compute global data array bounds");

  // Let the svtkPKdTree class compile global bounds for all
  // data arrays.  These can be accessed by D3 user by getting
  // a handle to the svtkPKdTree object and querying it.

  this->Kdtree->CreateGlobalDataArrayBounds();

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Redistribute data");

  // Stage (2) - Redistribute data, so that each process gets a ugrid
  //   containing the cells in it's assigned spatial regions.  (Note
  //   that a side effect of merging the grids received from different
  //   processes is that the final grid has no duplicate points.)
  //
  // This call will delete splitInput if it's not this->GetInput().

  svtkUnstructuredGrid* redistributedInput =
    this->RedistributeDataSet(splitInput, input, duplicateCells);

  if (redistributedInput == nullptr)
  {
    this->Kdtree->Delete();
    this->Kdtree = nullptr;

    svtkErrorMacro(<< "svtkPDistributedDataFilter::Execute redistribute failure");
    return 1;
  }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

  // Stage (3) - Add ghost cells to my sub grid.

  svtkUnstructuredGrid* expandedGrid = redistributedInput;

  if (this->GhostLevel > 0)
  {
    // Create global nodes IDs if we don't have them

    if (this->GetGlobalNodeIdArray(redistributedInput) == nullptr)
    {
      this->SetProgressText("Assign global point IDs");
      int rc = this->AssignGlobalNodeIds(redistributedInput);
      if (rc)
      {
        redistributedInput->Delete();
        this->Kdtree->Delete();
        this->Kdtree = nullptr;
        svtkErrorMacro(<< "svtkPDistributedDataFilter::Execute global node id creation");
        return 1;
      }
    }

    // redistributedInput will be deleted by AcquireGhostCells

    this->SetProgressText("Exchange ghost cells");
    expandedGrid = this->AcquireGhostCells(redistributedInput);
  }

  // Stage (4) - Clip cells to the spatial region boundaries

  if (this->ClipCells)
  {
    this->SetProgressText("Clip boundary cells");
    this->ClipGridCells(expandedGrid);
    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  }

  // remove temporary arrays we created

  this->SetProgressText("Clean up and finish");

  svtkDataArray* da = expandedGrid->GetCellData()->GetArray(TEMP_ELEMENT_ID_NAME);

  if (da)
  {
    expandedGrid->GetCellData()->RemoveArray(TEMP_ELEMENT_ID_NAME);
  }

  da = expandedGrid->GetPointData()->GetArray(TEMP_NODE_ID_NAME);

  if (da)
  {
    expandedGrid->GetCellData()->RemoveArray(TEMP_NODE_ID_NAME);
  }

  output->ShallowCopy(expandedGrid);
  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  expandedGrid->Delete();

  if (!this->RetainKdtree)
  {
    this->Kdtree->Delete();
    this->Kdtree = nullptr;
  }
  else
  {
    this->Kdtree->SetDataSet(nullptr);
  }

  this->UpdateProgress(1);

  return 1;
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::RedistributeDataSet(
  svtkDataSet* set, svtkDataSet* input, int filterOutDuplicateCells)
{
  TimeLog timer("RedistributeDataSet", this->Timing);
  (void)timer;

  // Create global cell ids before redistributing data.  These
  // will be necessary if we need ghost cells later on.

  svtkDataSet* inputPlus = set;

  if ((this->GhostLevel > 0) && (this->GetGlobalElementIdArray(set) == nullptr))
  {
    if (set == input)
    {
      inputPlus = set->NewInstance();
      inputPlus->ShallowCopy(set);
    }

    this->AssignGlobalElementIds(inputPlus);
  }

  // next call deletes inputPlus at the earliest opportunity

  svtkUnstructuredGrid* finalGrid = this->MPIRedistribute(inputPlus, input, filterOutDuplicateCells);

  return finalGrid;
}

//----------------------------------------------------------------------------
int svtkPDistributedDataFilter::PartitionDataAndAssignToProcesses(svtkDataSet* set)
{
  TimeLog timer("PartitionDataAndAssignToProcesses", this->Timing);
  (void)timer;

  if (this->Kdtree == nullptr)
  {
    this->Kdtree = svtkPKdTree::New();
    if (!this->UserCuts)
    {
      this->Kdtree->AssignRegionsContiguous();
    }
    this->Kdtree->SetTiming(this->GetTiming());
  }
  if (this->UserCuts)
  {
    this->Kdtree->SetCuts(this->UserCuts);
  }

  this->Kdtree->SetController(this->Controller);
  this->Kdtree->SetNumberOfRegionsOrMore(this->NumProcesses);
  this->Kdtree->SetMinCells(0);
  this->Kdtree->SetDataSet(set);

  // BuildLocator is smart enough to rebuild the k-d tree only if
  // the input geometry has changed, or the k-d tree build parameters
  // have changed.  It will reassign regions if the region assignment
  // scheme has changed.

  this->Kdtree->BuildLocator();

  int nregions = this->Kdtree->GetNumberOfRegions();

  if (nregions < this->NumProcesses)
  {
    if (nregions == 0)
    {
      svtkErrorMacro("Unable to build k-d tree structure");
    }
    else
    {
      svtkErrorMacro(<< "K-d tree must have at least one region per process.  "
                    << "Needed " << this->NumProcesses << ", has " << nregions);
    }
    this->Kdtree->Delete();
    this->Kdtree = nullptr;
    return 1;
  }

  if (this->UserRegionAssignments.size() > 0)
  {
    if (static_cast<int>(this->UserRegionAssignments.size()) != nregions)
    {
      svtkWarningMacro("Mismatch in number of user-defined regions and regions"
                      " the in KdTree. Ignoring user-defined regions.");
    }
    else
    {
      this->Kdtree->AssignRegions(&this->UserRegionAssignments[0], nregions);
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int svtkPDistributedDataFilter::ClipGridCells(svtkUnstructuredGrid* grid)
{
  TimeLog timer("ClipGridCells", this->Timing);
  (void)timer;

  if (grid->GetNumberOfCells() == 0)
  {
    return 0;
  }

  // Global point IDs are meaningless after
  // clipping, since this tetrahedralizes the whole data set.
  // We remove that array.

  if (this->GetGlobalNodeIdArray(grid))
  {
    grid->GetPointData()->SetGlobalIds(nullptr);
  }

  this->ClipCellsToSpatialRegion(grid);

  return 0;
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::AcquireGhostCells(svtkUnstructuredGrid* grid)
{
  TimeLog timer("AcquireGhostCells", this->Timing);
  (void)timer;

  if (this->GhostLevel < 1)
  {
    return grid;
  }

  // Create a search structure mapping global point IDs to local point IDs

  svtkIdType numPoints = grid->GetNumberOfPoints();

  svtkIdType* gnids = nullptr;

  if (numPoints > 0)
  {
    gnids = this->GetGlobalNodeIds(grid);

    if (!gnids)
    {
      svtkWarningMacro(<< "Can't create ghost cells without global node IDs");
      return grid;
    }
  }

  svtkPDistributedDataFilterSTLCloak globalToLocalMap;

  for (int localPtId = 0; localPtId < numPoints; localPtId++)
  {
    const int id = gnids[localPtId];
    globalToLocalMap.IntMap.insert(std::pair<const int, int>(id, localPtId));
  }

  svtkUnstructuredGrid* expandedGrid = nullptr;

  if (this->IncludeAllIntersectingCells)
  {
    expandedGrid = this->AddGhostCellsDuplicateCellAssignment(grid, &globalToLocalMap);
  }
  else
  {
    expandedGrid = this->AddGhostCellsUniqueCellAssignment(grid, &globalToLocalMap);
  }

  convertGhostLevelsToBitFields(expandedGrid->GetCellData(), svtkDataSetAttributes::DUPLICATECELL);
  convertGhostLevelsToBitFields(expandedGrid->GetPointData(), svtkDataSetAttributes::DUPLICATEPOINT);

  return expandedGrid;
}

//----------------------------------------------------------------------------
void svtkPDistributedDataFilter::SingleProcessExecute(svtkDataSet* input, svtkUnstructuredGrid* output)
{
  TimeLog timer("SingleProcessExecute", this->Timing);
  (void)timer;

  svtkDebugMacro(<< "svtkPDistributedDataFilter::SingleProcessExecute()");

  // we run the input through svtkMergeCells which will remove
  // duplicate points

  svtkDataSet* tmp = input->NewInstance();
  tmp->ShallowCopy(input);

  float tolerance = 0.0;

  if (this->RetainKdtree)
  {
    if (this->Kdtree == nullptr)
    {
      this->Kdtree = svtkPKdTree::New();
      if (this->UserCuts)
      {
        this->Kdtree->SetCuts(this->UserCuts);
      }
      this->Kdtree->SetTiming(this->GetTiming());
    }

    this->Kdtree->SetDataSet(tmp);
    this->Kdtree->BuildLocator();
    tolerance = (float)this->Kdtree->GetFudgeFactor();
    this->Kdtree->CreateGlobalDataArrayBounds();
  }
  else if (this->Kdtree)
  {
    this->Kdtree->Delete();
    this->Kdtree = nullptr;
  }

  svtkUnstructuredGrid* clean =
    svtkPDistributedDataFilter::MergeGrids(&tmp, 1, DeleteYes, 1, tolerance, 0);

  output->ShallowCopy(clean);
  clean->Delete();

  if (this->GhostLevel > 0)
  {
    // Add the svtkGhostType arrays.  We have the whole
    // data set, so all cells are level 0.

    svtkPDistributedDataFilter::AddConstantUnsignedCharPointArray(
      output, svtkDataSetAttributes::GhostArrayName(), 0);
    svtkPDistributedDataFilter::AddConstantUnsignedCharCellArray(
      output, svtkDataSetAttributes::GhostArrayName(), 0);
  }
}

//----------------------------------------------------------------------------
void svtkPDistributedDataFilter::ComputeMyRegionBounds()
{
  delete[] this->ConvexSubRegionBounds;
  this->ConvexSubRegionBounds = nullptr;

  svtkIntArray* myRegions = svtkIntArray::New();

  this->Kdtree->GetRegionAssignmentList(this->MyId, myRegions);

  if (myRegions->GetNumberOfTuples() > 0)
  {
    this->NumConvexSubRegions =
      this->Kdtree->MinimalNumberOfConvexSubRegions(myRegions, &this->ConvexSubRegionBounds);
  }
  else
  {
    this->NumConvexSubRegions = 0;
  }

  myRegions->Delete();
}

//----------------------------------------------------------------------------
int svtkPDistributedDataFilter::CheckFieldArrayTypes(svtkDataSet* set)
{
  int i;

  // problem - svtkIdType arrays are written out as int arrays
  // when marshalled with svtkDataWriter.  This is a problem
  // when receive the array and try to merge it with our own,
  // which is a svtkIdType

  svtkPointData* pd = set->GetPointData();
  svtkCellData* cd = set->GetCellData();

  int npointArrays = pd->GetNumberOfArrays();

  for (i = 0; i < npointArrays; i++)
  {
    int arrayType = pd->GetArray(i)->GetDataType();

    if (arrayType == SVTK_ID_TYPE)
    {
      return 1;
    }
  }

  int ncellArrays = cd->GetNumberOfArrays();

  for (i = 0; i < ncellArrays; i++)
  {
    int arrayType = cd->GetArray(i)->GetDataType();

    if (arrayType == SVTK_ID_TYPE)
    {
      return 1;
    }
  }

  return 0;
}

//-------------------------------------------------------------------------
// Quickly spread input data around if there are more processes than
// input data sets.
//-------------------------------------------------------------------------
struct svtkPDistributedDataFilterProcInfo
{
  svtkIdType had;
  int procId;
  svtkIdType has;
};
extern "C"
{
  int svtkPDistributedDataFilterSortSize(const void* s1, const void* s2)
  {
    svtkPDistributedDataFilterProcInfo *a, *b;

    a = (struct svtkPDistributedDataFilterProcInfo*)s1;
    b = (struct svtkPDistributedDataFilterProcInfo*)s2;

    if (a->has < b->has)
    {
      return 1;
    }
    else if (a->has == b->has)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }
}

//----------------------------------------------------------------------------
svtkDataSet* svtkPDistributedDataFilter::TestFixTooFewInputFiles(
  svtkDataSet* input, int& duplicateCells)
{
  TimeLog timer("TestFixTooFewInputFiles", this->Timing);
  (void)timer;

  svtkIdType i;
  int proc;
  int me = this->MyId;
  int nprocs = this->NumProcesses;

  svtkIdType numMyCells = input->GetNumberOfCells();

  // Find out how many input cells each process has.

  svtkIdTypeArray* inputSize = this->ExchangeCounts(numMyCells, 0x0001);
  svtkIdType* sizes = inputSize->GetPointer(0);

  int* nodeType = new int[nprocs];
  const int Producer = 1;
  const int Consumer = 2;
  int numConsumers = 0;
  svtkIdType numTotalCells = 0;

  for (proc = 0; proc < nprocs; proc++)
  {
    numTotalCells += sizes[proc];
    if (sizes[proc] == 0)
    {
      numConsumers++;
      nodeType[proc] = Consumer;
    }
    else
    {
      nodeType[proc] = Producer;
    }
  }

  if (numTotalCells == 0)
  {
    // Nothing to do.
    // Based on the comments in RequestData() where this method is called, if
    // this method returns nullptr, it indicates that there's no distribution to be
    // done. That's indeed the case for empty datasets. Hence we'll return nullptr.
    delete[] nodeType;
    inputSize->Delete();
    return nullptr;
  }

  if (numConsumers == 0)
  {
    // Nothing to do.  Every process has input data.

    delete[] nodeType;
    inputSize->Delete();
    return input;
  }

  // if nb of cells is lower than nb of procs, some cells will be duplicated
  duplicateCells = (numTotalCells < nprocs ? DuplicateCellsYes : DuplicateCellsNo);

  // compute global cell ids to handle cells duplication
  svtkSmartPointer<svtkDataSet> inputPlus = input;
  if (duplicateCells == DuplicateCellsYes && this->GetGlobalElementIdArray(input) == nullptr)
  {
    inputPlus = svtkSmartPointer<svtkDataSet>::NewInstance(input);
    inputPlus->ShallowCopy(input);
    this->AssignGlobalElementIds(inputPlus);
  }

  svtkIdType cellsPerNode = numTotalCells / nprocs;

  svtkIdList** sendCells = new svtkIdList*[nprocs];
  memset(sendCells, 0, sizeof(svtkIdList*) * nprocs);

  if (numConsumers == nprocs - 1)
  {
    // Simple and common case.
    // Only one process has data and divides it among the rest.

    inputSize->Delete();

    if (nodeType[me] == Producer)
    {
      if (numTotalCells < nprocs)
      {
        // If there are not enough cells to go around, just give one cell
        // to each process, duplicating as necessary.
        for (proc = 0; proc < nprocs; proc++)
        {
          sendCells[proc] = svtkIdList::New();
          sendCells[proc]->SetNumberOfIds(1);
          sendCells[proc]->SetId(0, proc % numTotalCells);
        }
      }
      else
      {
        svtkIdType sizeLast = numTotalCells - ((nprocs - 1) * cellsPerNode);
        svtkIdType cellId = 0;

        for (proc = 0; proc < nprocs; proc++)
        {
          svtkIdType ncells = ((proc == nprocs - 1) ? sizeLast : cellsPerNode);

          sendCells[proc] = svtkIdList::New();
          sendCells[proc]->SetNumberOfIds(ncells);

          for (i = 0; i < ncells; i++)
          {
            sendCells[proc]->SetId(i, cellId++);
          }
        }
      }
    }
  }
  else
  {
    if (numTotalCells < nprocs)
    {
      for (proc = 0; nodeType[proc] != Producer; proc++)
      {
        // empty loop.
      }
      if (proc == me)
      {
        // Have one process give out its cells to consumers.
        svtkIdType numCells = inputSize->GetValue(me);
        i = 0;
        sendCells[me] = svtkIdList::New();
        sendCells[me]->SetNumberOfIds(1);
        sendCells[me]->SetId(0, i++);
        if (i >= numCells)
          i = 0;
        for (proc = 0; proc < nprocs; proc++)
        {
          if (nodeType[proc] == Consumer)
          {
            sendCells[proc] = svtkIdList::New();
            sendCells[proc]->SetNumberOfIds(1);
            sendCells[proc]->SetId(0, i++);
            if (i >= numCells)
              i = 0;
          }
        }
      }
      else if (nodeType[me] == Producer)
      {
        // All other producers keep their own cells.
        svtkIdType numCells = inputSize->GetValue(me);
        sendCells[me] = svtkIdList::New();
        sendCells[me]->SetNumberOfIds(numCells);
        for (i = 0; i < numCells; i++)
        {
          sendCells[me]->SetId(i, i);
        }
      }

      inputSize->Delete();
    }
    else
    {

      // The processes with data send it to processes without data.
      // This is not the most balanced decomposition, and it is not the
      // fastest.  It is somewhere in between.

      svtkIdType minCells = (svtkIdType)(.8 * (double)cellsPerNode);

      struct svtkPDistributedDataFilterProcInfo* procInfo =
        new struct svtkPDistributedDataFilterProcInfo[nprocs];

      for (proc = 0; proc < nprocs; proc++)
      {
        procInfo[proc].had = inputSize->GetValue(proc);
        procInfo[proc].procId = proc;
        procInfo[proc].has = inputSize->GetValue(proc);
      }

      inputSize->Delete();

      qsort(procInfo, nprocs, sizeof(struct svtkPDistributedDataFilterProcInfo),
        svtkPDistributedDataFilterSortSize);

      struct svtkPDistributedDataFilterProcInfo* nextProducer = procInfo;
      struct svtkPDistributedDataFilterProcInfo* nextConsumer = procInfo + (nprocs - 1);

      svtkIdType numTransferCells = 0;

      int sanityCheck = 0;
      int nprocsSquared = nprocs * nprocs;

      while (sanityCheck++ < nprocsSquared)
      {
        int c = nextConsumer->procId;

        if (nodeType[c] == Producer)
        {
          break;
        }

        svtkIdType cGetMin = minCells - nextConsumer->has;

        if (cGetMin < 1)
        {
          nextConsumer--;
          continue;
        }
        svtkIdType cGetMax = cellsPerNode - nextConsumer->has;

        int p = nextProducer->procId;

        svtkIdType pSendMax = nextProducer->has - minCells;

        if (pSendMax < 1)
        {
          nextProducer++;
          continue;
        }

        svtkIdType transferSize = (pSendMax < cGetMax) ? pSendMax : cGetMax;

        if (me == p)
        {
          svtkIdType startCellId = nextProducer->had - nextProducer->has;
          sendCells[c] = svtkIdList::New();
          sendCells[c]->SetNumberOfIds(transferSize);
          for (i = 0; i < transferSize; i++)
          {
            sendCells[c]->SetId(i, startCellId++);
          }

          numTransferCells += transferSize;
        }

        nextProducer->has -= transferSize;
        nextConsumer->has += transferSize;

        continue;
      }

      delete[] procInfo;

      if (sanityCheck > nprocsSquared)
      {
        svtkErrorMacro(<< "TestFixTooFewInputFiles error");
        for (i = 0; i < nprocs; i++)
        {
          if (sendCells[i])
          {
            sendCells[i]->Delete();
          }
        }
        delete[] sendCells;
        delete[] nodeType;
        sendCells = nullptr;
      }
      else if (nodeType[me] == Producer)
      {
        svtkIdType keepCells = numMyCells - numTransferCells;
        svtkIdType startCellId = (svtkIdType)numTransferCells;
        sendCells[me] = svtkIdList::New();
        sendCells[me]->SetNumberOfIds(keepCells);
        for (i = 0; i < keepCells; i++)
        {
          sendCells[me]->SetId(i, startCellId++);
        }
      }
    }
  }

  svtkUnstructuredGrid* newGrid = nullptr;

  if (sendCells)
  {
    newGrid = this->ExchangeMergeSubGrids(
      sendCells, DeleteYes, inputPlus, DeleteNo, DuplicateCellsNo, GhostCellsNo, 0x0011);

    delete[] sendCells;
    delete[] nodeType;
  }

  return newGrid;
}

//============================================================================
// Communication routines - two versions:
//   *Lean version use minimal memory
//   *Fast versions use more memory, but are much faster

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::SetUpPairWiseExchange()
{
  TimeLog timer("SetUpPairWiseExchange", this->Timing);
  (void)timer;

  int iam = this->MyId;
  int nprocs = this->NumProcesses;

  delete[] this->Target;
  this->Target = nullptr;

  delete[] this->Source;
  this->Source = nullptr;

  if (nprocs == 1)
  {
    return;
  }

  this->Target = new int[nprocs - 1];
  this->Source = new int[nprocs - 1];

  for (int i = 1; i < nprocs; i++)
  {
    this->Target[i - 1] = (iam + i) % nprocs;
    this->Source[i - 1] = (iam + nprocs - i) % nprocs;
  }
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::FreeIntArrays(svtkIdTypeArray** ar)
{
  for (int i = 0; i < this->NumProcesses; i++)
  {
    if (ar[i])
    {
      ar[i]->Delete();
    }
  }

  delete[] ar;
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::FreeIdLists(svtkIdList** lists, int nlists)
{
  for (int i = 0; i < nlists; i++)
  {
    if (lists[i])
    {
      lists[i]->Delete();
      lists[i] = nullptr;
    }
  }
}

//-------------------------------------------------------------------------
svtkIdType svtkPDistributedDataFilter::GetIdListSize(svtkIdList** lists, int nlists)
{
  svtkIdType numCells = 0;

  for (int i = 0; i < nlists; i++)
  {
    if (lists[i])
    {
      numCells += lists[i]->GetNumberOfIds();
    }
  }

  return numCells;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExchangeMergeSubGrids(svtkIdList** cellIds,
  int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells,
  int ghostCellFlag, int tag)
{
  TimeLog timer("ExchangeMergeSubGrids(1)", this->Timing);
  (void)timer;

  int nprocs = this->NumProcesses;

  int* numLists = new int[nprocs];

  svtkIdList*** listOfLists = new svtkIdList**[nprocs];

  for (int i = 0; i < nprocs; i++)
  {
    if (cellIds[i] == nullptr)
    {
      numLists[i] = 0;
    }
    else
    {
      numLists[i] = 1;
    }

    listOfLists[i] = &cellIds[i];
  }

  svtkUnstructuredGrid* grid = nullptr;

  if (this->UseMinimalMemory)
  {
    grid = this->ExchangeMergeSubGridsLean(listOfLists, numLists, deleteCellIds, myGrid,
      deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
  }
  else
  {
    grid = this->ExchangeMergeSubGridsFast(listOfLists, numLists, deleteCellIds, myGrid,
      deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
  }

  delete[] numLists;
  delete[] listOfLists;

  return grid;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExchangeMergeSubGrids(svtkIdList*** cellIds,
  int* numLists, int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid,
  int filterOutDuplicateCells, int ghostCellFlag, int tag)
{
  TimeLog timer("ExchangeMergeSubGrids(2)", this->Timing);
  (void)timer;

  svtkUnstructuredGrid* grid = nullptr;

  if (this->UseMinimalMemory)
  {
    grid = this->ExchangeMergeSubGridsLean(cellIds, numLists, deleteCellIds, myGrid, deleteMyGrid,
      filterOutDuplicateCells, ghostCellFlag, tag);
  }
  else
  {
    grid = this->ExchangeMergeSubGridsFast(cellIds, numLists, deleteCellIds, myGrid, deleteMyGrid,
      filterOutDuplicateCells, ghostCellFlag, tag);
  }
  return grid;
}

//-------------------------------------------------------------------------
svtkIdTypeArray* svtkPDistributedDataFilter::ExchangeCounts(svtkIdType myCount, int tag)
{
  svtkIdTypeArray* ia;

  if (this->UseMinimalMemory)
  {
    ia = this->ExchangeCountsLean(myCount, tag);
  }
  else
  {
    ia = this->ExchangeCountsFast(myCount, tag);
  }
  return ia;
}

//-------------------------------------------------------------------------
svtkFloatArray** svtkPDistributedDataFilter::ExchangeFloatArrays(
  svtkFloatArray** myArray, int deleteSendArrays, int tag)
{
  svtkFloatArray** fa;

  if (this->UseMinimalMemory)
  {
    fa = this->ExchangeFloatArraysLean(myArray, deleteSendArrays, tag);
  }
  else
  {
    fa = this->ExchangeFloatArraysFast(myArray, deleteSendArrays, tag);
  }
  return fa;
}

//-------------------------------------------------------------------------
svtkIdTypeArray** svtkPDistributedDataFilter::ExchangeIdArrays(
  svtkIdTypeArray** myArray, int deleteSendArrays, int tag)
{
  svtkIdTypeArray** ia;

  if (this->UseMinimalMemory)
  {
    ia = this->ExchangeIdArraysLean(myArray, deleteSendArrays, tag);
  }
  else
  {
    ia = this->ExchangeIdArraysFast(myArray, deleteSendArrays, tag);
  }
  return ia;
}

// ----------------------- Lean versions ----------------------------//
svtkIdTypeArray* svtkPDistributedDataFilter::ExchangeCountsLean(svtkIdType myCount, int tag)
{
  svtkIdTypeArray* countArray = nullptr;

  svtkIdType i;
  int nprocs = this->NumProcesses;

  svtkMPICommunicator::Request req;
  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  svtkIdType* counts = new svtkIdType[nprocs];
  counts[this->MyId] = myCount;

  if (!this->Source)
  {
    this->SetUpPairWiseExchange();
  }

  for (i = 0; i < this->NumProcesses - 1; i++)
  {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(counts + source, 1, source, tag, req);
    mpiContr->Send(&myCount, 1, target, tag);
    req.Wait();
  }

  countArray = svtkIdTypeArray::New();
  countArray->SetArray(counts, nprocs, 0, svtkIdTypeArray::SVTK_DATA_ARRAY_DELETE);

  return countArray;
}

//-------------------------------------------------------------------------
svtkFloatArray** svtkPDistributedDataFilter::ExchangeFloatArraysLean(
  svtkFloatArray** myArray, int deleteSendArrays, int tag)
{
  svtkFloatArray** remoteArrays = nullptr;

  int i;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  svtkMPICommunicator::Request req;
  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  int* recvSize = new int[nprocs];
  int* sendSize = new int[nprocs];

  if (!this->Source)
  {
    this->SetUpPairWiseExchange();
  }

  for (i = 0; i < nprocs; i++)
  {
    sendSize[i] = myArray[i] ? myArray[i]->GetNumberOfTuples() : 0;
    recvSize[i] = 0;
  }

  // Exchange sizes

  int nothers = nprocs - 1;

  for (i = 0; i < nothers; i++)
  {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(recvSize + source, 1, source, tag, req);
    mpiContr->Send(sendSize + target, 1, target, tag);
    req.Wait();
  }

  // Exchange int arrays

  float** recvArrays = new float*[nprocs];
  memset(recvArrays, 0, sizeof(float*) * nprocs);

  if (sendSize[me] > 0) // sent myself an array
  {
    recvSize[me] = sendSize[me];
    recvArrays[me] = new float[sendSize[me]];
    memcpy(recvArrays[me], myArray[me]->GetPointer(0), sendSize[me] * sizeof(float));
  }

  for (i = 0; i < nothers; i++)
  {
    int source = this->Source[i];
    int target = this->Target[i];
    recvArrays[source] = nullptr;

    if (recvSize[source] > 0)
    {
      recvArrays[source] = new float[recvSize[source]];
      if (recvArrays[source] == nullptr)
      {
        svtkErrorMacro(<< "svtkPDistributedDataFilter::ExchangeIdArrays memory allocation");
        delete[] recvSize;
        delete[] recvArrays;
        return nullptr;
      }
      mpiContr->NoBlockReceive(recvArrays[source], recvSize[source], source, tag, req);
    }

    if (sendSize[target] > 0)
    {
      mpiContr->Send(myArray[target]->GetPointer(0), sendSize[target], target, tag);
    }

    if (myArray[target] && deleteSendArrays)
    {
      myArray[target]->Delete();
    }

    if (recvSize[source] > 0)
    {
      req.Wait();
    }
  }

  if (deleteSendArrays)
  {
    if (myArray[me])
    {
      myArray[me]->Delete();
    }
    delete[] myArray;
  }

  delete[] sendSize;

  remoteArrays = new svtkFloatArray*[nprocs];

  for (i = 0; i < nprocs; i++)
  {
    if (recvSize[i] > 0)
    {
      remoteArrays[i] = svtkFloatArray::New();
      remoteArrays[i]->SetArray(recvArrays[i], recvSize[i], svtkFloatArray::SVTK_DATA_ARRAY_DELETE);
    }
    else
    {
      remoteArrays[i] = nullptr;
    }
  }

  delete[] recvArrays;
  delete[] recvSize;

  return remoteArrays;
}

//-------------------------------------------------------------------------
svtkIdTypeArray** svtkPDistributedDataFilter::ExchangeIdArraysLean(
  svtkIdTypeArray** myArray, int deleteSendArrays, int tag)
{
  svtkIdTypeArray** remoteArrays = nullptr;

  int i;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  svtkMPICommunicator::Request req;
  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  svtkIdType* recvSize = new svtkIdType[nprocs];
  svtkIdType* sendSize = new svtkIdType[nprocs];

  if (!this->Source)
  {
    this->SetUpPairWiseExchange();
  }

  for (i = 0; i < nprocs; i++)
  {
    sendSize[i] = myArray[i] ? myArray[i]->GetNumberOfTuples() : 0;
    recvSize[i] = 0;
  }

  // Exchange sizes

  int nothers = nprocs - 1;

  for (i = 0; i < nothers; i++)
  {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(recvSize + source, 1, source, tag, req);
    mpiContr->Send(sendSize + target, 1, target, tag);
    req.Wait();
  }

  // Exchange int arrays

  svtkIdType** recvArrays = new svtkIdType*[nprocs];
  memset(recvArrays, 0, sizeof(svtkIdType*) * nprocs);

  if (sendSize[me] > 0) // sent myself an array
  {
    recvSize[me] = sendSize[me];
    recvArrays[me] = new svtkIdType[sendSize[me]];
    memcpy(recvArrays[me], myArray[me]->GetPointer(0), sendSize[me] * sizeof(svtkIdType));
  }

  for (i = 0; i < nothers; i++)
  {
    int source = this->Source[i];
    int target = this->Target[i];
    recvArrays[source] = nullptr;

    if (recvSize[source] > 0)
    {
      recvArrays[source] = new svtkIdType[recvSize[source]];
      if (recvArrays[source] == nullptr)
      {
        svtkErrorMacro(<< "svtkPDistributedDataFilter::ExchangeIdArrays memory allocation");
        delete[] sendSize;
        delete[] recvSize;
        delete[] recvArrays;
        return nullptr;
      }
      mpiContr->NoBlockReceive(recvArrays[source], recvSize[source], source, tag, req);
    }

    if (sendSize[target] > 0)
    {
      mpiContr->Send(myArray[target]->GetPointer(0), sendSize[target], target, tag);
    }

    if (myArray[target] && deleteSendArrays)
    {
      myArray[target]->Delete();
    }

    if (recvSize[source] > 0)
    {
      req.Wait();
    }
  }

  if (deleteSendArrays)
  {
    if (myArray[me])
    {
      myArray[me]->Delete();
    }
    delete[] myArray;
  }

  delete[] sendSize;

  remoteArrays = new svtkIdTypeArray*[nprocs];

  for (i = 0; i < nprocs; i++)
  {
    if (recvSize[i] > 0)
    {
      remoteArrays[i] = svtkIdTypeArray::New();
      remoteArrays[i]->SetArray(
        recvArrays[i], recvSize[i], 0, svtkIdTypeArray::SVTK_DATA_ARRAY_DELETE);
    }
    else
    {
      remoteArrays[i] = nullptr;
    }
  }

  delete[] recvArrays;
  delete[] recvSize;

  return remoteArrays;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExchangeMergeSubGridsLean(svtkIdList*** cellIds,
  int* numLists, int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid,
  int filterOutDuplicateCells,   // flag if different processes may send same cells
  int svtkNotUsed(ghostCellFlag), // flag if these cells are ghost cells
  int tag)
{
  TimeLog timer("ExchangeMergeSubGridsLean", this->Timing);
  (void)timer;

  svtkUnstructuredGrid* mergedGrid = nullptr;
  int i;
  svtkIdType packedGridSendSize = 0, packedGridRecvSize = 0;
  char *packedGridSend = nullptr, *packedGridRecv = nullptr;
  int recvBufSize = 0;
  int numReceivedGrids = 0;

  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);
  svtkMPICommunicator::Request req;

  svtkDataSet* tmpGrid = myGrid->NewInstance();
  tmpGrid->ShallowCopy(myGrid);

  svtkDataSet** grids = new svtkDataSet*[nprocs];

  if (numLists[iam] > 0)
  {
    // I was extracting/packing/sending/unpacking ugrids of zero cells,
    // and this caused corrupted data structures.  I don't know why, but
    // I am now being careful not to do that.

    svtkIdType numCells = svtkPDistributedDataFilter::GetIdListSize(cellIds[iam], numLists[iam]);

    if (numCells > 0)
    {
      grids[numReceivedGrids++] =
        this->ExtractCells(cellIds[iam], numLists[iam], deleteCellIds, tmpGrid);
    }
    else if (deleteCellIds)
    {
      svtkPDistributedDataFilter::FreeIdLists(cellIds[iam], numLists[iam]);
    }
  }

  if (this->Source == nullptr)
  {
    this->SetUpPairWiseExchange();
  }

  int nothers = nprocs - 1;

  for (i = 0; i < nothers; i++)
  {
    int target = this->Target[i];
    int source = this->Source[i];

    packedGridSendSize = 0;

    if (cellIds[target] && (numLists[target] > 0))
    {
      svtkIdType numCells =
        svtkPDistributedDataFilter::GetIdListSize(cellIds[target], numLists[target]);

      if (numCells > 0)
      {
        svtkUnstructuredGrid* sendGrid =
          this->ExtractCells(cellIds[target], numLists[target], deleteCellIds, tmpGrid);

        packedGridSend = this->MarshallDataSet(sendGrid, packedGridSendSize);
        sendGrid->Delete();
      }
      else if (deleteCellIds)
      {
        svtkPDistributedDataFilter::FreeIdLists(cellIds[target], numLists[target]);
      }
    }

    // exchange size of packed grids

    mpiContr->NoBlockReceive(&packedGridRecvSize, 1, source, tag, req);
    mpiContr->Send(&packedGridSendSize, 1, target, tag);
    req.Wait();

    if (packedGridRecvSize > recvBufSize)
    {
      delete[] packedGridRecv;
      packedGridRecv = new char[packedGridRecvSize];
      if (!packedGridRecv)
      {
        svtkErrorMacro(<< "svtkPDistributedDataFilter::ExchangeMergeSubGrids memory allocation");
        delete[] grids;
        return nullptr;
      }
      recvBufSize = packedGridRecvSize;
    }

    if (packedGridRecvSize > 0)
    {
      mpiContr->NoBlockReceive(packedGridRecv, packedGridRecvSize, source, tag, req);
    }

    if (packedGridSendSize > 0)
    {
      mpiContr->Send(packedGridSend, packedGridSendSize, target, tag);
      delete[] packedGridSend;
    }

    if (packedGridRecvSize > 0)
    {
      req.Wait();

      grids[numReceivedGrids++] = this->UnMarshallDataSet(packedGridRecv, packedGridRecvSize);
    }
  }

  tmpGrid->Delete();

  if (recvBufSize > 0)
  {
    delete[] packedGridRecv;
    packedGridRecv = nullptr;
  }

  if (numReceivedGrids > 1)
  {
    // Merge received grids

    // this call will merge the grids and then delete them

    float tolerance = 0.0;

    if (this->Kdtree)
    {
      tolerance = (float)this->Kdtree->GetFudgeFactor();
    }

    mergedGrid = svtkPDistributedDataFilter::MergeGrids(
      grids, numReceivedGrids, DeleteYes, 1, tolerance, filterOutDuplicateCells);
  }
  else if (numReceivedGrids == 1)
  {
    mergedGrid = svtkUnstructuredGrid::SafeDownCast(grids[0]);
  }
  else
  {
    mergedGrid = this->ExtractZeroCellGrid(myGrid);
  }

  if (deleteMyGrid)
  {
    myGrid->Delete();
  }

  delete[] grids;

  return mergedGrid;
}

// ----------------------- Fast versions ----------------------------//
svtkIdTypeArray* svtkPDistributedDataFilter::ExchangeCountsFast(
  svtkIdType myCount, int svtkNotUsed(tag))
{
  int nprocs = this->NumProcesses;

  svtkIdType* counts = new svtkIdType[nprocs];
  this->Controller->AllGather(&myCount, counts, 1);

  svtkIdTypeArray* countArray = svtkIdTypeArray::New();
  countArray->SetArray(counts, nprocs, 0, svtkIdTypeArray::SVTK_DATA_ARRAY_DELETE);
  return countArray;
}

//-------------------------------------------------------------------------
svtkFloatArray** svtkPDistributedDataFilter::ExchangeFloatArraysFast(
  svtkFloatArray** myArray, int deleteSendArrays, int tag)
{
  svtkFloatArray** fa = nullptr;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  int* sendSize = new int[nprocs];
  int* recvSize = new int[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    recvSize[proc] = sendSize[proc] = 0;

    if (proc == iam)
    {
      continue;
    }

    if (myArray[proc])
    {
      sendSize[proc] = myArray[proc]->GetNumberOfTuples();
    }
  }

  // Exchange sizes of arrays to send and receive

  svtkMPICommunicator::Request* reqBuf = new svtkMPICommunicator::Request[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
  }

  mpiContr->Barrier();

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
  }

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    reqBuf[proc].Wait();
  }

  // Allocate buffers and post receives

  float** recvBufs = new float*[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (recvSize[proc] > 0)
    {
      recvBufs[proc] = new float[recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
    }
    else
    {
      recvBufs[proc] = nullptr;
    }
  }

  mpiContr->Barrier();

  // Send all arrays

  for (proc = 0; proc < nprocs; proc++)
  {
    if (sendSize[proc] > 0)
    {
      mpiContr->Send(myArray[proc]->GetPointer(0), sendSize[proc], proc, tag);
    }
  }
  delete[] sendSize;

  // If I want to send an array to myself, place it in output now

  if (myArray[iam])
  {
    recvSize[iam] = myArray[iam]->GetNumberOfTuples();
    if (recvSize[iam] > 0)
    {
      recvBufs[iam] = new float[recvSize[iam]];
      memcpy(recvBufs[iam], myArray[iam]->GetPointer(0), recvSize[iam] * sizeof(float));
    }
  }

  if (deleteSendArrays)
  {
    for (proc = 0; proc < nprocs; proc++)
    {
      if (myArray[proc])
      {
        myArray[proc]->Delete();
      }
    }
    delete[] myArray;
  }

  // Await incoming arrays

  fa = new svtkFloatArray*[nprocs];
  for (proc = 0; proc < nprocs; proc++)
  {
    if (recvBufs[proc])
    {
      fa[proc] = svtkFloatArray::New();
      fa[proc]->SetArray(recvBufs[proc], recvSize[proc], 0, svtkFloatArray::SVTK_DATA_ARRAY_DELETE);
    }
    else
    {
      fa[proc] = nullptr;
    }
  }

  delete[] recvSize;

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    if (recvBufs[proc])
    {
      reqBuf[proc].Wait();
    }
  }

  delete[] reqBuf;
  delete[] recvBufs;

  return fa;
}

//-------------------------------------------------------------------------
svtkIdTypeArray** svtkPDistributedDataFilter::ExchangeIdArraysFast(
  svtkIdTypeArray** myArray, int deleteSendArrays, int tag)
{
  svtkIdTypeArray** ia = nullptr;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  svtkIdType* sendSize = new svtkIdType[nprocs];
  svtkIdType* recvSize = new svtkIdType[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    recvSize[proc] = sendSize[proc] = 0;

    if (proc == iam)
    {
      continue;
    }

    if (myArray[proc])
    {
      sendSize[proc] = myArray[proc]->GetNumberOfTuples();
    }
  }

  // Exchange sizes of arrays to send and receive

  svtkMPICommunicator::Request* reqBuf = new svtkMPICommunicator::Request[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
  }

  mpiContr->Barrier();

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
  }

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    reqBuf[proc].Wait();
  }

  // Allocate buffers and post receives

  svtkIdType** recvBufs = new svtkIdType*[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (recvSize[proc] > 0)
    {
      recvBufs[proc] = new svtkIdType[recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
    }
    else
    {
      recvBufs[proc] = nullptr;
    }
  }

  mpiContr->Barrier();

  // Send all arrays

  for (proc = 0; proc < nprocs; proc++)
  {
    if (sendSize[proc] > 0)
    {
      mpiContr->Send(myArray[proc]->GetPointer(0), sendSize[proc], proc, tag);
    }
  }
  delete[] sendSize;

  // If I want to send an array to myself, place it in output now

  if (myArray[iam])
  {
    recvSize[iam] = myArray[iam]->GetNumberOfTuples();
    if (recvSize[iam] > 0)
    {
      recvBufs[iam] = new svtkIdType[recvSize[iam]];
      memcpy(recvBufs[iam], myArray[iam]->GetPointer(0), recvSize[iam] * sizeof(svtkIdType));
    }
  }

  if (deleteSendArrays)
  {
    for (proc = 0; proc < nprocs; proc++)
    {
      if (myArray[proc])
      {
        myArray[proc]->Delete();
      }
    }
    delete[] myArray;
  }

  // Await incoming arrays

  ia = new svtkIdTypeArray*[nprocs];
  for (proc = 0; proc < nprocs; proc++)
  {
    if (recvBufs[proc])
    {
      ia[proc] = svtkIdTypeArray::New();
      ia[proc]->SetArray(recvBufs[proc], recvSize[proc], 0, svtkIdTypeArray::SVTK_DATA_ARRAY_DELETE);
    }
    else
    {
      ia[proc] = nullptr;
    }
  }

  delete[] recvSize;

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    if (recvBufs[proc])
    {
      reqBuf[proc].Wait();
    }
  }

  delete[] reqBuf;
  delete[] recvBufs;

  return ia;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExchangeMergeSubGridsFast(svtkIdList*** cellIds,
  int* numLists, int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid,
  int filterOutDuplicateCells,   // flag if different processes may send same cells
  int svtkNotUsed(ghostCellFlag), // flag if these are ghost cells
  int tag)
{
  TimeLog timer("ExchangeMergeSubGridsFast", this->Timing);
  (void)timer;

  svtkUnstructuredGrid* mergedGrid = nullptr;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  svtkMPIController* mpiContr = svtkMPIController::SafeDownCast(this->Controller);

  svtkUnstructuredGrid** grids = new svtkUnstructuredGrid*[nprocs];
  char** sendBufs = new char*[nprocs];
  char** recvBufs = new char*[nprocs];
  svtkIdType* sendSize = new svtkIdType[nprocs];
  svtkIdType* recvSize = new svtkIdType[nprocs];

  // create & pack all sub grids

  TimeLog::StartEvent("Create & pack all sub grids", this->Timing);

  svtkDataSet* tmpGrid = myGrid->NewInstance();
  tmpGrid->ShallowCopy(myGrid);

  for (proc = 0; proc < nprocs; proc++)
  {
    recvSize[proc] = sendSize[proc] = 0;
    grids[proc] = nullptr;
    sendBufs[proc] = recvBufs[proc] = nullptr;

    if (numLists[proc] > 0)
    {
      svtkIdType numCells = svtkPDistributedDataFilter::GetIdListSize(cellIds[proc], numLists[proc]);

      if (numCells > 0)
      {
        grids[proc] = svtkPDistributedDataFilter::ExtractCells(
          cellIds[proc], numLists[proc], deleteCellIds, tmpGrid);

        if (proc != iam)
        {
          sendBufs[proc] = this->MarshallDataSet(grids[proc], sendSize[proc]);
          grids[proc]->Delete();
          grids[proc] = nullptr;
        }
      }
      else if (deleteCellIds)
      {
        svtkPDistributedDataFilter::FreeIdLists(cellIds[proc], numLists[proc]);
      }
    }
  }

  tmpGrid->Delete();

  TimeLog::EndEvent("Create & pack all sub grids", this->Timing);

  // Exchange sizes of grids to send and receive

  svtkMPICommunicator::Request* reqBuf = new svtkMPICommunicator::Request[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
  }

  mpiContr->Barrier();

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
  }

  for (proc = 0; proc < nprocs; proc++)
  {
    if (proc == iam)
    {
      continue;
    }
    reqBuf[proc].Wait();
  }

  // Allocate buffers and post receives

  int numReceives = 0;

  for (proc = 0; proc < nprocs; proc++)
  {
    if (recvSize[proc] > 0)
    {
      recvBufs[proc] = new char[recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
      numReceives++;
    }
  }

  mpiContr->Barrier();

  // Send all sub grids, then delete them

  TimeLog::StartEvent("Send all sub grids", this->Timing);

  for (proc = 0; proc < nprocs; proc++)
  {
    if (sendSize[proc] > 0)
    {
      mpiContr->Send(sendBufs[proc], sendSize[proc], proc, tag);
    }
  }

  for (proc = 0; proc < nprocs; proc++)
  {
    if (sendSize[proc] > 0)
    {
      delete[] sendBufs[proc];
    }
  }

  delete[] sendSize;
  delete[] sendBufs;

  TimeLog::EndEvent("Send all sub grids", this->Timing);

  // Await incoming sub grids, unpack them

  TimeLog::StartEvent("Receive and unpack incoming sub grids", this->Timing);

  while (numReceives > 0)
  {
    for (proc = 0; proc < nprocs; proc++)
    {
      if (recvBufs[proc] && (reqBuf[proc].Test() == 1))
      {
        grids[proc] = this->UnMarshallDataSet(recvBufs[proc], recvSize[proc]);
        delete[] recvBufs[proc];
        recvBufs[proc] = nullptr;
        numReceives--;
      }
    }
  }

  delete[] reqBuf;
  delete[] recvBufs;
  delete[] recvSize;

  TimeLog::EndEvent("Receive and unpack incoming sub grids", this->Timing);

  // Merge received grids

  TimeLog::StartEvent("Merge received grids", this->Timing);

  float tolerance = 0.0;

  if (this->Kdtree)
  {
    tolerance = (float)this->Kdtree->GetFudgeFactor();
  }

  int numReceivedGrids = 0;

  svtkDataSet** ds = new svtkDataSet*[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    if (grids[proc] != nullptr)
    {
      ds[numReceivedGrids++] = static_cast<svtkDataSet*>(grids[proc]);
    }
  }

  delete[] grids;

  if (numReceivedGrids > 1)
  {
    // Normally, using this->GetGlobalNodeIds is the right thing.  However,
    // there is a bit of a bug here that this filter only works with ids
    // that are svtkIdType.  Otherwise, it will return nullptr as the global ids.
    // That is bad because then the global node ids will be stripped in the
    // MergeGrids method, and the number of point arrays will not match,
    // causing a crash latter on.
    // int useGlobalNodeIds = (this->GetGlobalNodeIds(ds[0]) != nullptr);
    int useGlobalNodeIds = (ds[0]->GetPointData()->GetGlobalIds() != nullptr);

    // this call will merge the grids and then delete them
    TimeLog timer2("MergeGrids", this->Timing);
    (void)timer2;

    mergedGrid = svtkPDistributedDataFilter::MergeGrids(
      ds, numReceivedGrids, DeleteYes, useGlobalNodeIds, tolerance, filterOutDuplicateCells);
  }
  else if (numReceivedGrids == 1)
  {
    mergedGrid = svtkUnstructuredGrid::SafeDownCast(ds[0]);
  }
  else
  {
    mergedGrid = this->ExtractZeroCellGrid(myGrid);
  }

  if (deleteMyGrid)
  {
    myGrid->Delete();
  }

  delete[] ds;

  TimeLog::EndEvent("Merge received grids", this->Timing);

  return mergedGrid;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::MPIRedistribute(
  svtkDataSet* in, svtkDataSet* input, int filterOutDuplicateCells)
{
  TimeLog timer("MPIRedistribute", this->Timing);
  (void)timer;

  int proc;
  int nprocs = this->NumProcesses;

  // A cell belongs to a spatial region if it's centroid lies in that
  // region.  The kdtree object can create a list for each region of the
  // IDs of each cell I have read in that belong in that region.  If we
  // are building subgrids of all cells that intersect a region (a
  // superset of all cells that belong to a region) then the kdtree object
  // can build another set of lists of all cells that intersect each
  // region (but don't have their centroid in that region).

  if (this->IncludeAllIntersectingCells)
  {
    // TO DO:
    // We actually compute whether a cell intersects a spatial region.
    // This can be a lengthy calculation.  Perhaps it's good enough
    // to compute whether a cell's bounding box intersects the region.
    // Some of the cells we list will actually not be in the region, but
    // if we are clipping later, it doesn't matter.
    //
    // Is there any rendering algorithm that needs exactly all cells
    // which intersect the region, and no more?

    this->Kdtree->IncludeRegionBoundaryCellsOn(); // SLOW!!
  }

  this->Kdtree->CreateCellLists(); // required by GetCellIdsForProcess

  svtkIdList*** procCellLists = new svtkIdList**[nprocs];
  int* numLists = new int[nprocs];

  for (proc = 0; proc < this->NumProcesses; proc++)
  {
    procCellLists[proc] = this->GetCellIdsForProcess(proc, numLists + proc);
  }

  int deleteDataSet = DeleteNo;

  if (in != input)
  {
    deleteDataSet = DeleteYes;
  }

  svtkUnstructuredGrid* myNewGrid = this->ExchangeMergeSubGrids(procCellLists, numLists, DeleteNo,
    in, deleteDataSet, filterOutDuplicateCells, GhostCellsNo, 0x0012);

  for (proc = 0; proc < nprocs; proc++)
  {
    delete[] procCellLists[proc];
  }

  delete[] procCellLists;
  delete[] numLists;

  if (myNewGrid && (this->GhostLevel > 0))
  {
    svtkPDistributedDataFilter::AddConstantUnsignedCharCellArray(
      myNewGrid, svtkDataSetAttributes::GhostArrayName(), 0);
    svtkPDistributedDataFilter::AddConstantUnsignedCharPointArray(
      myNewGrid, svtkDataSetAttributes::GhostArrayName(), 0);
  }
  return myNewGrid;
}

//-------------------------------------------------------------------------
char* svtkPDistributedDataFilter::MarshallDataSet(svtkUnstructuredGrid* extractedGrid, svtkIdType& len)
{
  TimeLog timer("MarshallDataSet", this->Timing);
  (void)timer;

  // taken from svtkCommunicator::WriteDataSet

  svtkUnstructuredGrid* copy;
  svtkDataSetWriter* writer = svtkDataSetWriter::New();

  copy = extractedGrid->NewInstance();
  copy->ShallowCopy(extractedGrid);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() > 0)
  {
    writer->SetFileTypeToBinary();
  }
  writer->WriteToOutputStringOn();
  writer->SetInputData(copy);

  writer->Write();

  len = writer->GetOutputStringLength();

  char* packedFormat = writer->RegisterAndGetOutputString();

  writer->Delete();

  copy->Delete();

  return packedFormat;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::UnMarshallDataSet(char* buf, svtkIdType size)
{
  TimeLog timer("UnMarshallDataSet", this->Timing);
  (void)timer;

  // taken from svtkCommunicator::ReadDataSet

  svtkDataSetReader* reader = svtkDataSetReader::New();

  reader->ReadFromInputStringOn();

  svtkCharArray* mystring = svtkCharArray::New();

  mystring->SetArray(buf, size, 1);

  reader->SetInputArray(mystring);
  mystring->Delete();

  svtkDataSet* output = reader->GetOutput();
  reader->Update();

  svtkUnstructuredGrid* newGrid = svtkUnstructuredGrid::New();

  newGrid->ShallowCopy(output);

  reader->Delete();

  return newGrid;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExtractCells(
  svtkIdList* cells, int deleteCellLists, svtkDataSet* in)
{
  TimeLog timer("ExtractCells(1)", this->Timing);
  (void)timer;

  svtkIdList* tempCellList = nullptr;

  if (cells == nullptr)
  {
    // We'll get a zero cell unstructured grid which matches the input grid
    tempCellList = svtkIdList::New();
  }
  else
  {
    tempCellList = cells;
  }

  svtkUnstructuredGrid* subGrid =
    svtkPDistributedDataFilter::ExtractCells(&tempCellList, 1, deleteCellLists, in);

  if (tempCellList != cells)
  {
    tempCellList->Delete();
  }

  return subGrid;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExtractCells(
  svtkIdList** cells, int nlists, int deleteCellLists, svtkDataSet* in)
{
  TimeLog timer("ExtractCells(2)", this->Timing);
  (void)timer;

  svtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  svtkExtractCells* extCells = svtkExtractCells::New();

  extCells->SetInputData(tmpInput);

  for (int i = 0; i < nlists; i++)
  {
    if (cells[i])
    {
      extCells->AddCellList(cells[i]);

      if (deleteCellLists)
      {
        cells[i]->Delete();
      }
    }
  }

  extCells->Update();

  // If this process has no cells for these regions, a ugrid gets
  // created anyway with field array information

  svtkUnstructuredGrid* keepGrid = svtkUnstructuredGrid::New();
  keepGrid->ShallowCopy(extCells->GetOutput());

  extCells->Delete();

  tmpInput->Delete();

  return keepGrid;
}

//-------------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::ExtractZeroCellGrid(svtkDataSet* in)
{
  TimeLog timer("ExtractZeroCellGrid", this->Timing);
  (void)timer;

  svtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  svtkExtractCells* extCells = svtkExtractCells::New();

  extCells->SetInputData(tmpInput);

  extCells->Update(); // extract no cells

  svtkUnstructuredGrid* keepGrid = svtkUnstructuredGrid::New();
  keepGrid->ShallowCopy(extCells->GetOutput());

  extCells->Delete();

  tmpInput->Delete();

  return keepGrid;
}

//-------------------------------------------------------------------------

// To save on storage, we return actual pointers into the svtkKdTree's lists
// of cell IDs.  So don't free the memory they are pointing to.
// svtkKdTree::DeleteCellLists will delete them all when we're done.

svtkIdList** svtkPDistributedDataFilter::GetCellIdsForProcess(int proc, int* nlists)
{
  TimeLog timer("GetCellIdsForProcess", this->Timing);
  (void)timer;

  *nlists = 0;

  svtkIntArray* regions = svtkIntArray::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

  if (nregions == 0)
  {
    return nullptr;
  }

  *nlists = nregions;

  if (this->IncludeAllIntersectingCells)
  {
    *nlists *= 2;
  }

  svtkIdList** lists = new svtkIdList*[*nlists];

  int nextList = 0;

  for (int reg = 0; reg < nregions; reg++)
  {
    lists[nextList++] = this->Kdtree->GetCellList(regions->GetValue(reg));

    if (this->IncludeAllIntersectingCells)
    {
      lists[nextList++] = this->Kdtree->GetBoundaryCellList(regions->GetValue(reg));
    }
  }

  regions->Delete();

  return lists;
}

//==========================================================================
// Code related to clipping cells to the spatial region

//-------------------------------------------------------------------------
static int insideBoxFunction(svtkIdType cellId, svtkUnstructuredGrid* grid, void* data)
{
  char* arrayName = (char*)data;

  svtkDataArray* da = grid->GetCellData()->GetArray(arrayName);
  svtkUnsignedCharArray* inside = svtkArrayDownCast<svtkUnsignedCharArray>(da);

  unsigned char where = inside->GetValue(cellId);

  return where; // 1 if cell is inside spatial region, 0 otherwise
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::AddConstantUnsignedCharPointArray(
  svtkUnstructuredGrid* grid, const char* arrayName, unsigned char val)
{
  svtkUnsignedCharArray* Array = svtkUnsignedCharArray::New();
  Array->SetName(arrayName);

  svtkIdType npoints = grid->GetNumberOfPoints();
  if (npoints)
  {
    unsigned char* vals = new unsigned char[npoints];
    memset(vals, val, npoints);

    Array->SetArray(vals, npoints, 0, svtkUnsignedCharArray::SVTK_DATA_ARRAY_DELETE);
  }

  grid->GetPointData()->AddArray(Array);

  Array->Delete();
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::AddConstantUnsignedCharCellArray(
  svtkUnstructuredGrid* grid, const char* arrayName, unsigned char val)
{
  svtkUnsignedCharArray* Array = svtkUnsignedCharArray::New();
  Array->SetName(arrayName);

  svtkIdType ncells = grid->GetNumberOfCells();
  if (ncells)
  {
    unsigned char* vals = new unsigned char[ncells];
    memset(vals, val, ncells);

    Array->SetArray(vals, ncells, 0, svtkUnsignedCharArray::SVTK_DATA_ARRAY_DELETE);
  }

  grid->GetCellData()->AddArray(Array);

  Array->Delete();
}

#if 0
//-------------------------------------------------------------------------
// this is here temporarily, until svtkBoxClipDataSet is fixed to
// be able to generate the clipped output
void svtkPDistributedDataFilter::ClipWithVtkClipDataSet(
           svtkUnstructuredGrid *grid, double *bounds,
           svtkUnstructuredGrid **outside, svtkUnstructuredGrid **inside)
{
  svtkUnstructuredGrid *in;
  svtkUnstructuredGrid *out ;

  svtkClipDataSet *clipped = svtkClipDataSet::New();

  svtkBox *box = svtkBox::New();
  box->SetBounds(bounds);

  clipped->SetClipFunction(box);
  box->Delete();
  clipped->SetValue(0.0);
  clipped->InsideOutOn();

  clipped->SetInputData(grid);

  if (outside)
  {
    clipped->GenerateClippedOutputOn();
  }

  clipped->Update();

  if (outside)
  {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
  }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;


  clipped->Delete();
}
#endif

//-------------------------------------------------------------------------
// In general, svtkBoxClipDataSet is much faster and makes fewer errors.
void svtkPDistributedDataFilter::ClipWithBoxClipDataSet(svtkUnstructuredGrid* grid, double* bounds,
  svtkUnstructuredGrid** outside, svtkUnstructuredGrid** inside)
{
  TimeLog timer("ClipWithBoxClipDataSet", this->Timing);
  (void)timer;

  svtkUnstructuredGrid* in;
  svtkUnstructuredGrid* out;

  svtkBoxClipDataSet* clipped = svtkBoxClipDataSet::New();

  clipped->SetBoxClip(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);

  clipped->SetInputData(grid);

  if (outside)
  {
    clipped->GenerateClippedOutputOn();
  }

  clipped->Update();

  if (outside)
  {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
  }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;

  clipped->Delete();
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::ClipCellsToSpatialRegion(svtkUnstructuredGrid* grid)
{
  TimeLog timer("ClipCellsToSpatialRegion", this->Timing);
  (void)timer;

  this->ComputeMyRegionBounds();

  if (this->NumConvexSubRegions > 1)
  {
    // here we would need to divide the grid into a separate grid for
    // each convex region, and then do the clipping

    svtkErrorMacro(<< "svtkPDistributedDataFilter::ClipCellsToSpatialRegion - "
                     "assigned regions do not form a single convex region");

    return;
  }

  double* bounds = this->ConvexSubRegionBounds;

  if (this->GhostLevel > 0)
  {
    // We need cells outside the clip box as well.

    svtkUnstructuredGrid* outside;
    svtkUnstructuredGrid* inside;

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, &outside, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, &outside, &inside);
#endif

    grid->Initialize();

    // Mark the outside cells with a 0, the inside cells with a 1.

    int arrayNameLen = static_cast<int>(strlen(TEMP_INSIDE_BOX_FLAG));
    char* arrayName = new char[arrayNameLen + 1];
    strcpy(arrayName, TEMP_INSIDE_BOX_FLAG);
    svtkPDistributedDataFilter::AddConstantUnsignedCharCellArray(outside, arrayName, 0);
    svtkPDistributedDataFilter::AddConstantUnsignedCharCellArray(inside, arrayName, 1);

    // Combine inside and outside into a single ugrid.

    svtkDataSet* grids[2];
    grids[0] = inside;
    grids[1] = outside;

    svtkUnstructuredGrid* combined = svtkPDistributedDataFilter::MergeGrids(
      grids, 2, DeleteYes, 0, (float)this->Kdtree->GetFudgeFactor(), 0);

    // Extract the piece inside the box (level 0) and the requested
    // number of levels of ghost cells.

    svtkExtractUserDefinedPiece* ep = svtkExtractUserDefinedPiece::New();

    ep->SetConstantData(arrayName, arrayNameLen + 1);
    ep->SetPieceFunction(insideBoxFunction);
    ep->CreateGhostCellsOn();

    ep->GetExecutive()->GetOutputInformation(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    ep->SetInputData(combined);

    ep->Update();

    grid->ShallowCopy(ep->GetOutput());
    grid->GetCellData()->RemoveArray(arrayName);

    ep->Delete();
    combined->Delete();

    delete[] arrayName;
  }
  else
  {
    svtkUnstructuredGrid* inside;

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, nullptr, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, nullptr, &inside);
#endif

    grid->ShallowCopy(inside);
    inside->Delete();
  }

  return;
}

//==========================================================================
// Code related to assigning global node IDs and cell IDs

//-------------------------------------------------------------------------
int svtkPDistributedDataFilter::AssignGlobalNodeIds(svtkUnstructuredGrid* grid)
{
  TimeLog timer("AssignGlobalNodeIds", this->Timing);
  (void)timer;

  int nprocs = this->NumProcesses;
  int pid;
  svtkIdType ptId;
  svtkIdType nGridPoints = grid->GetNumberOfPoints();

  svtkIdType* numPointsOutside = new svtkIdType[nprocs];
  memset(numPointsOutside, 0, sizeof(svtkIdType) * nprocs);

  svtkIdTypeArray* globalIds = svtkIdTypeArray::New();
  globalIds->SetNumberOfValues(nGridPoints);
  globalIds->SetName(TEMP_NODE_ID_NAME);

  // 1. Count the points in grid which lie within my assigned spatial region

  svtkIdType myNumPointsInside = 0;

  for (ptId = 0; ptId < nGridPoints; ptId++)
  {
    double* pt = grid->GetPoints()->GetPoint(ptId);

    if (this->InMySpatialRegion(pt[0], pt[1], pt[2]))
    {
      globalIds->SetValue(ptId, 0); // flag it as mine
      myNumPointsInside++;
    }
    else
    {
      // Well, whose region is this point in?

      int regionId = this->Kdtree->GetRegionContainingPoint(pt[0], pt[1], pt[2]);

      pid = this->Kdtree->GetProcessAssignedToRegion(regionId);

      numPointsOutside[pid]++;

      pid += 1;
      pid *= -1;

      globalIds->SetValue(ptId, pid); // a flag
    }
  }

  // 2. Gather and Broadcast this number of "Inside" points for each process.

  svtkIdTypeArray* numPointsInside = this->ExchangeCounts(myNumPointsInside, 0x0013);

  // 3. Assign global Ids to the points inside my spatial region

  svtkIdType firstId = 0;
  svtkIdType numGlobalIdsSoFar = 0;

  for (pid = 0; pid < nprocs; pid++)
  {
    if (pid < this->MyId)
    {
      firstId += numPointsInside->GetValue(pid);
    }
    numGlobalIdsSoFar += numPointsInside->GetValue(pid);
  }

  numPointsInside->Delete();

  for (ptId = 0; ptId < nGridPoints; ptId++)
  {
    if (globalIds->GetValue(ptId) == 0)
    {
      globalIds->SetValue(ptId, firstId++);
    }
  }

  // -----------------------------------------------------------------
  // All processes have assigned global IDs to the points in their grid
  // which lie within their assigned spatial region.
  // Now they have to get the IDs for the
  // points in their grid which lie outside their region, and which
  // are within the spatial region of another process.
  // -----------------------------------------------------------------

  // 4. For every other process, build a list of points I have
  // which are in the region of that process.  In practice, the
  // processes for which I need to request points IDs should be
  // a small subset of all the other processes.

  // question: if the svtkPointArray has type double, should we
  // send doubles instead of floats to insure we get the right
  // global ID back?

  svtkFloatArray** ptarrayOut = new svtkFloatArray*[nprocs];
  memset(ptarrayOut, 0, sizeof(svtkFloatArray*) * nprocs);

  svtkIdTypeArray** localIds = new svtkIdTypeArray*[nprocs];
  memset(localIds, 0, sizeof(svtkIdTypeArray*) * nprocs);

  svtkIdType* next = new svtkIdType[nprocs];
  svtkIdType* next3 = new svtkIdType[nprocs];

  for (ptId = 0; ptId < nGridPoints; ptId++)
  {
    pid = globalIds->GetValue(ptId);

    if (pid >= 0)
    {
      continue; // that's one of mine
    }

    pid *= -1;
    pid -= 1;

    if (ptarrayOut[pid] == nullptr)
    {
      svtkIdType npoints = numPointsOutside[pid];

      ptarrayOut[pid] = svtkFloatArray::New();
      ptarrayOut[pid]->SetNumberOfValues(npoints * 3);

      localIds[pid] = svtkIdTypeArray::New();
      localIds[pid]->SetNumberOfValues(npoints);

      next[pid] = 0;
      next3[pid] = 0;
    }

    localIds[pid]->SetValue(next[pid]++, ptId);

    double* dp = grid->GetPoints()->GetPoint(ptId);

    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[0]);
    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[1]);
    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[2]);
  }

  delete[] numPointsOutside;
  delete[] next;
  delete[] next3;

  // 5. Do pairwise exchanges of the points we want global IDs for,
  //    and delete outgoing point arrays.

  svtkFloatArray** ptarrayIn = this->ExchangeFloatArrays(ptarrayOut, DeleteYes, 0x0014);

  // 6. Find the global point IDs that have been requested of me,
  //    and delete incoming point arrays.  Count "missing points":
  //    the number of unique points I receive which are not in my
  //    grid (this may happen if IncludeAllIntersectingCells is OFF).

  svtkIdType myNumMissingPoints = 0;

  svtkIdTypeArray** idarrayOut =
    this->FindGlobalPointIds(ptarrayIn, globalIds, grid, myNumMissingPoints);

  svtkIdTypeArray* missingCount = this->ExchangeCounts(myNumMissingPoints, 0x0015);

  if (this->IncludeAllIntersectingCells == 1)
  {
    // Make sure all points were found

    int aok = 1;
    for (pid = 0; pid < nprocs; pid++)
    {
      if (missingCount->GetValue(pid) > 0)
      {
        svtkErrorMacro(<< "svtkPDistributedDataFilter::AssignGlobalNodeIds bad point");
        aok = 0;
        break;
      }
    }
    if (!aok)
    {
      this->FreeIntArrays(idarrayOut);
      this->FreeIntArrays(localIds);
      missingCount->Delete();
      globalIds->Delete();

      return 1;
    }
  }

  // 7. Do pairwise exchanges of the global point IDs, and delete the
  //    outgoing point ID arrays.

  svtkIdTypeArray** idarrayIn = this->ExchangeIdArrays(idarrayOut, DeleteYes, 0x0016);

  // 8. It's possible (if IncludeAllIntersectingCells is OFF) that some
  //    processes had "missing points".  Process A has a point P in it's
  //    grid which lies in the spatial region of process B.  But P is not
  //    in process B's grid.  We need to assign global IDs to these points
  //    too.

  svtkIdType* missingId = new svtkIdType[nprocs];

  if (this->IncludeAllIntersectingCells == 0)
  {
    missingId[0] = numGlobalIdsSoFar;

    for (pid = 1; pid < nprocs; pid++)
    {
      int prev = pid - 1;
      missingId[pid] = missingId[prev] + missingCount->GetValue(prev);
    }
  }

  missingCount->Delete();

  // 9. Update my ugrid with these mutually agreed upon global point IDs

  for (pid = 0; pid < nprocs; pid++)
  {
    if (idarrayIn[pid] == nullptr)
    {
      continue;
    }

    svtkIdType count = idarrayIn[pid]->GetNumberOfTuples();

    for (ptId = 0; ptId < count; ptId++)
    {
      svtkIdType myLocalId = localIds[pid]->GetValue(ptId);
      svtkIdType yourGlobalId = idarrayIn[pid]->GetValue(ptId);

      if (yourGlobalId >= 0)
      {
        globalIds->SetValue(myLocalId, yourGlobalId);
      }
      else
      {
        svtkIdType ptIdOffset = yourGlobalId * -1;
        ptIdOffset -= 1;

        globalIds->SetValue(myLocalId, missingId[pid] + ptIdOffset);
      }
    }
    localIds[pid]->Delete();
    idarrayIn[pid]->Delete();
  }

  delete[] localIds;
  delete[] idarrayIn;
  delete[] missingId;

  grid->GetPointData()->SetGlobalIds(globalIds);
  globalIds->Delete();

  return 0;
}

//-------------------------------------------------------------------------
// If grids were distributed with IncludeAllIntersectingCells OFF, it's
// possible there are points in my spatial region that are not in my
// grid.  They need global Ids, so I will keep track of how many such unique
// points I receive from other processes, and will assign them temporary
// IDs.  They will get permanent IDs later on.

svtkIdTypeArray** svtkPDistributedDataFilter::FindGlobalPointIds(svtkFloatArray** ptarray,
  svtkIdTypeArray* ids, svtkUnstructuredGrid* grid, svtkIdType& numUniqueMissingPoints)
{
  TimeLog timer("FindGlobalPointIds", this->Timing);
  (void)timer;

  int nprocs = this->NumProcesses;
  svtkIdTypeArray** gids = new svtkIdTypeArray*[nprocs];

  if (grid->GetNumberOfCells() == 0)
  {
    // There are no cells in my assigned region

    memset(gids, 0, sizeof(svtkIdTypeArray*) * nprocs);

    return gids;
  }

  svtkKdTree* kd = svtkKdTree::New();

  kd->BuildLocatorFromPoints(grid->GetPoints());

  int procId;
  svtkIdType ptId, localId;

  svtkPointLocator* pl = nullptr;
  svtkPoints* missingPoints = nullptr;

  if (this->IncludeAllIntersectingCells == 0)
  {
    this->ComputeMyRegionBounds();
    pl = svtkPointLocator::New();
    pl->SetTolerance(this->Kdtree->GetFudgeFactor());
    missingPoints = svtkPoints::New();
    pl->InitPointInsertion(missingPoints, this->ConvexSubRegionBounds);
  }

  for (procId = 0; procId < nprocs; procId++)
  {
    if ((ptarray[procId] == nullptr) || (ptarray[procId]->GetNumberOfTuples() == 0))
    {
      gids[procId] = nullptr;
      if (ptarray[procId])
        ptarray[procId]->Delete();
      continue;
    }

    gids[procId] = svtkIdTypeArray::New();

    svtkIdType npoints = ptarray[procId]->GetNumberOfTuples() / 3;

    gids[procId]->SetNumberOfValues(npoints);
    svtkIdType next = 0;

    float* pt = ptarray[procId]->GetPointer(0);

    for (ptId = 0; ptId < npoints; ptId++)
    {
      localId = kd->FindPoint((double)pt[0], (double)pt[1], (double)pt[2]);

      if (localId >= 0)
      {
        gids[procId]->SetValue(next++, ids->GetValue(localId)); // global Id
      }
      else
      {
        // This point is not in my grid

        if (this->IncludeAllIntersectingCells)
        {
          // This is an error
          gids[procId]->SetValue(next++, -1);
          numUniqueMissingPoints++;
        }
        else
        {
          // Flag these with a negative point ID.  We'll assign
          // them real point IDs later.

          svtkIdType nextId;
          double dpt[3];
          dpt[0] = pt[0];
          dpt[1] = pt[1];
          dpt[2] = pt[2];
          pl->InsertUniquePoint(dpt, nextId);

          nextId += 1;
          nextId *= -1;
          gids[procId]->SetValue(next++, nextId);
        }
      }
      pt += 3;
    }

    ptarray[procId]->Delete();
  }

  delete[] ptarray;

  kd->Delete();

  if (missingPoints)
  {
    numUniqueMissingPoints = missingPoints->GetNumberOfPoints();
    missingPoints->Delete();
    pl->Delete();
  }

  return gids;
}

//-------------------------------------------------------------------------
int svtkPDistributedDataFilter::AssignGlobalElementIds(svtkDataSet* in)
{
  TimeLog timer("AssignGlobalElementIds", this->Timing);
  (void)timer;

  svtkIdType i;
  svtkIdType myNumCells = in->GetNumberOfCells();
  svtkIdTypeArray* numCells = this->ExchangeCounts(myNumCells, 0x0017);

  svtkIdTypeArray* globalCellIds = svtkIdTypeArray::New();
  globalCellIds->SetNumberOfValues(myNumCells);
  // DDM - do we need to mark this as the GID array?
  globalCellIds->SetName(TEMP_ELEMENT_ID_NAME);

  svtkIdType StartId = 0;

  for (i = 0; i < this->MyId; i++)
  {
    StartId += numCells->GetValue(i);
  }

  numCells->Delete();

  for (i = 0; i < myNumCells; i++)
  {
    globalCellIds->SetValue(i, StartId++);
  }

  in->GetCellData()->SetGlobalIds(globalCellIds);

  globalCellIds->Delete();

  return 0;
}

//========================================================================
// Code related to acquiring ghost cells

//-------------------------------------------------------------------------
int svtkPDistributedDataFilter::InMySpatialRegion(float x, float y, float z)
{
  return this->InMySpatialRegion((double)x, (double)y, (double)z);
}
int svtkPDistributedDataFilter::InMySpatialRegion(double x, double y, double z)
{
  this->ComputeMyRegionBounds();

  double* box = this->ConvexSubRegionBounds;

  if (!box)
  {
    return 0;
  }

  // To avoid ambiguity, a point on a boundary is assigned to
  // the region for which it is on the upper boundary.  Or
  // (in one dimension) the region between points A and B
  // contains all points p such that A < p <= B.

  if ((x <= box[0]) || (x > box[1]) || (y <= box[2]) || (y > box[3]) || (z <= box[4]) ||
    (z > box[5]))
  {
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------
int svtkPDistributedDataFilter::StrictlyInsideMyBounds(float x, float y, float z)
{
  return this->StrictlyInsideMyBounds((double)x, (double)y, (double)z);
}

//-----------------------------------------------------------------------
int svtkPDistributedDataFilter::StrictlyInsideMyBounds(double x, double y, double z)
{
  this->ComputeMyRegionBounds();

  double* box = this->ConvexSubRegionBounds;

  if (!box)
  {
    return 0;
  }

  if ((x <= box[0]) || (x >= box[1]) || (y <= box[2]) || (y >= box[3]) || (z <= box[4]) ||
    (z >= box[5]))
  {
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------
svtkIdTypeArray** svtkPDistributedDataFilter::MakeProcessLists(
  svtkIdTypeArray** pointIds, svtkPDistributedDataFilterSTLCloak* procs)
{
  TimeLog timer("MakeProcessLists", this->Timing);
  (void)timer;

  // Build a list of pointId/processId pairs for each process that
  // sent me point IDs.  The process Ids are all those processes
  // that had the specified point in their ghost level zero grid.

  int nprocs = this->NumProcesses;

  std::multimap<int, int>::iterator mapIt;

  svtkIdTypeArray** processList = new svtkIdTypeArray*[nprocs];
  memset(processList, 0, sizeof(svtkIdTypeArray*) * nprocs);

  for (int i = 0; i < nprocs; i++)
  {
    if (pointIds[i] == nullptr)
    {
      continue;
    }

    svtkIdType size = pointIds[i]->GetNumberOfTuples();

    if (size > 0)
    {
      for (svtkIdType j = 0; j < size;)
      {
        // These are all the points in my spatial region
        // for which process "i" needs ghost cells.

        svtkIdType gid = pointIds[i]->GetValue(j);
        svtkIdType ncells = pointIds[i]->GetValue(j + 1);

        mapIt = procs->IntMultiMap.find(gid);

        while (mapIt != procs->IntMultiMap.end() && mapIt->first == gid)
        {
          int processId = mapIt->second;

          if (processId != i)
          {
            // Process "i" needs to know that process
            // "processId" also has cells using this point

            if (processList[i] == nullptr)
            {
              processList[i] = svtkIdTypeArray::New();
            }
            processList[i]->InsertNextValue(gid);
            processList[i]->InsertNextValue(processId);
          }
          ++mapIt;
        }
        j += (2 + ncells);
      }
    }
  }

  return processList;
}

//-----------------------------------------------------------------------
svtkIdTypeArray* svtkPDistributedDataFilter::AddPointAndCells(svtkIdType gid, svtkIdType localId,
  svtkUnstructuredGrid* grid, svtkIdType* gidCells, svtkIdTypeArray* ids)
{
  if (ids == nullptr)
  {
    ids = svtkIdTypeArray::New();
  }

  ids->InsertNextValue(gid);

  svtkIdList* cellList = svtkIdList::New();

  grid->GetPointCells(localId, cellList);

  svtkIdType numCells = cellList->GetNumberOfIds();

  ids->InsertNextValue(numCells);

  for (svtkIdType j = 0; j < numCells; j++)
  {
    svtkIdType globalCellId = gidCells[cellList->GetId(j)];
    ids->InsertNextValue(globalCellId);
  }

  cellList->Delete();

  return ids;
}

//-----------------------------------------------------------------------
svtkIdTypeArray** svtkPDistributedDataFilter::GetGhostPointIds(
  int ghostLevel, svtkUnstructuredGrid* grid, int AddCellsIAlreadyHave)
{
  TimeLog timer("GetGhostPointIds", this->Timing);
  (void)timer;

  int nprocs = this->NumProcesses;
  int me = this->MyId;
  svtkIdType numPoints = grid->GetNumberOfPoints();

  svtkIdTypeArray** ghostPtIds = new svtkIdTypeArray*[nprocs];
  memset(ghostPtIds, 0, sizeof(svtkIdTypeArray*) * nprocs);

  if (numPoints < 1)
  {
    return ghostPtIds;
  }

  int processId = -1;
  int regionId = -1;

  svtkPKdTree* kd = this->Kdtree;

  svtkPoints* pts = grid->GetPoints();

  svtkIdType* gidsPoint = this->GetGlobalNodeIds(grid);
  svtkIdType* gidsCell = this->GetGlobalElementIds(grid);

  svtkUnsignedCharArray* uca = grid->GetPointGhostArray();
  unsigned char* levels = uca->GetPointer(0);

  unsigned char level = (unsigned char)(ghostLevel - 1);

  for (svtkIdType i = 0; i < numPoints; i++)
  {
    double* pt = pts->GetPoint(i);
    regionId = kd->GetRegionContainingPoint(pt[0], pt[1], pt[2]);
    processId = kd->GetProcessAssignedToRegion(regionId);

    if (ghostLevel == 1)
    {
      // I want all points that are outside my spatial region

      if (processId == me)
      {
        continue;
      }

      // Don't include points that are not part of any cell

      int used = svtkPDistributedDataFilter::LocalPointIdIsUsed(grid, i);

      if (!used)
      {
        continue;
      }
    }
    else if (levels[i] != level)
    {
      continue; // I want all points having the correct ghost level
    }

    svtkIdType gid = gidsPoint[i];

    if (AddCellsIAlreadyHave)
    {
      // To speed up exchange of ghost cells and creation of
      // new ghost cell grid, we tell other
      // processes which cells we already have, so they don't
      // send them to us.

      ghostPtIds[processId] =
        svtkPDistributedDataFilter::AddPointAndCells(gid, i, grid, gidsCell, ghostPtIds[processId]);
    }
    else
    {
      if (ghostPtIds[processId] == nullptr)
      {
        ghostPtIds[processId] = svtkIdTypeArray::New();
      }
      ghostPtIds[processId]->InsertNextValue(gid);
      ghostPtIds[processId]->InsertNextValue(0);
    }
  }
  return ghostPtIds;
}

//-----------------------------------------------------------------------
int svtkPDistributedDataFilter::LocalPointIdIsUsed(svtkUnstructuredGrid* grid, int ptId)
{
  int used = 1;

  int numPoints = grid->GetNumberOfPoints();

  if ((ptId < 0) || (ptId >= numPoints))
  {
    used = 0;
  }
  else
  {
    svtkIdType id = (svtkIdType)ptId;
    svtkIdList* cellList = svtkIdList::New();

    grid->GetPointCells(id, cellList);

    if (cellList->GetNumberOfIds() == 0)
    {
      used = 0;
    }

    cellList->Delete();
  }

  return used;
}

//-----------------------------------------------------------------------
int svtkPDistributedDataFilter::GlobalPointIdIsUsed(
  svtkUnstructuredGrid* grid, int ptId, svtkPDistributedDataFilterSTLCloak* globalToLocal)
{
  int used = 1;

  std::map<int, int>::iterator mapIt;

  mapIt = globalToLocal->IntMap.find(ptId);

  if (mapIt == globalToLocal->IntMap.end())
  {
    used = 0;
  }
  else
  {
    int id = mapIt->second;

    used = svtkPDistributedDataFilter::LocalPointIdIsUsed(grid, id);
  }

  return used;
}

//-----------------------------------------------------------------------
svtkIdType svtkPDistributedDataFilter::FindId(svtkIdTypeArray* ids, svtkIdType gid, svtkIdType startLoc)
{
  svtkIdType gidLoc = -1;

  if (ids == nullptr)
  {
    return gidLoc;
  }

  svtkIdType numIds = ids->GetNumberOfTuples();

  while ((ids->GetValue(startLoc) != gid) && (startLoc < numIds))
  {
    svtkIdType ncells = ids->GetValue(++startLoc);
    startLoc += (ncells + 1);
  }

  if (startLoc < numIds)
  {
    gidLoc = startLoc;
  }

  return gidLoc;
}

//-----------------------------------------------------------------------
// We create an expanded grid with the required number of ghost
// cells.  This is for the case where IncludeAllIntersectingCells is OFF.
// This means that when the grid was redistributed, each cell was
// uniquely assigned to one process, the process owning the spatial
// region that the cell's centroid lies in.
svtkUnstructuredGrid* svtkPDistributedDataFilter::AddGhostCellsUniqueCellAssignment(
  svtkUnstructuredGrid* myGrid, svtkPDistributedDataFilterSTLCloak* globalToLocalMap)
{
  TimeLog timer("AddGhostCellsUniqueCellAssignment", this->Timing);
  (void)timer;

  int i, j, k;
  int ncells = 0;
  int processId = 0;
  int gid = 0;
  svtkIdType size = 0;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  svtkUnstructuredGrid* newGhostCellGrid = nullptr;
  svtkIdTypeArray** ghostPointIds = nullptr;

  svtkPDistributedDataFilterSTLCloak* insidePointMap = new svtkPDistributedDataFilterSTLCloak;
  std::multimap<int, int>::iterator mapIt;

  while (gl <= this->GhostLevel)
  {
    // For ghost level 1, create a list for each process (not
    // including me) of all points I have in that process'
    // assigned region.  We use this list for two purposes:
    // (1) to build a list on each process of all other processes
    // that have cells containing points in our region, (2)
    // these are some of the points that we need ghost cells for.
    //
    // For ghost level above 1, create a list for each process
    // (including me) of all my points in that process' assigned
    // region for which I need ghost cells.

    if (gl == 1)
    {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 0);
    }
    else
    {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
    }

    // Exchange these lists.

    svtkIdTypeArray** insideIds = this->ExchangeIdArrays(ghostPointIds, DeleteNo, 0x0018);

    if (gl == 1)
    {
      // For every point in my region that was sent to me by another process,
      // I now know the identity of all processes having cells containing
      // that point.  Begin by building a mapping from point IDs to the IDs
      // of processes that sent me that point.

      for (i = 0; i < nprocs; i++)
      {
        if (insideIds[i] == nullptr)
        {
          continue;
        }

        size = insideIds[i]->GetNumberOfTuples();

        if (size > 0)
        {
          for (j = 0; j < size; j += 2)
          {
            // map global point id to process ids
            const int id = (int)insideIds[i]->GetValue(j);
            insidePointMap->IntMultiMap.insert(std::pair<const int, int>(id, i));
          }
        }
      }
    }

    // Build a list of pointId/processId pairs for each process that
    // sent me point IDs.  To process P, for every point ID sent to me
    // by P, I send the ID of every other process (not including myself
    // and P) that has cells in it's ghost level 0 grid which use
    // this point.

    svtkIdTypeArray** processListSent = this->MakeProcessLists(insideIds, insidePointMap);

    // Exchange these new lists.

    svtkIdTypeArray** processList = this->ExchangeIdArrays(processListSent, DeleteYes, 0x0019);

    // I now know the identity of every process having cells containing
    // points I need ghost cells for.  Create a request to each process
    // for these cells.

    svtkIdTypeArray** ghostCellsPlease = new svtkIdTypeArray*[nprocs];
    for (i = 0; i < nprocs; i++)
    {
      ghostCellsPlease[i] = svtkIdTypeArray::New();
      ghostCellsPlease[i]->SetNumberOfComponents(1);
    }

    for (i = 0; i < nprocs; i++)
    {
      if (i == me)
      {
        continue;
      }

      if (ghostPointIds[i]) // points I have in your spatial region,
      {                     // maybe you have cells that use them?

        for (j = 0; j < ghostPointIds[i]->GetNumberOfTuples(); j++)
        {
          ghostCellsPlease[i]->InsertNextValue(ghostPointIds[i]->GetValue(j));
        }
      }
      if (processList[i]) // other processes you say that also have
      {                   // cells using those points
        size = processList[i]->GetNumberOfTuples();
        svtkIdType* array = processList[i]->GetPointer(0);
        int nextLoc = 0;

        for (j = 0; j < size; j += 2)
        {
          gid = array[j];
          processId = array[j + 1];

          ghostCellsPlease[processId]->InsertNextValue(gid);

          if (gl > 1)
          {
            // add the list of cells I already have for this point

            int where = svtkPDistributedDataFilter::FindId(ghostPointIds[i], gid, nextLoc);

            if (where < 0)
            {
              // error really, not sure what to do
              nextLoc = 0;
              ghostCellsPlease[processId]->InsertNextValue(0);
              continue;
            }

            ncells = ghostPointIds[i]->GetValue(where + 1);

            ghostCellsPlease[processId]->InsertNextValue(ncells);

            for (k = 0; k < ncells; k++)
            {
              svtkIdType cellId = ghostPointIds[i]->GetValue(where + 2 + k);
              ghostCellsPlease[processId]->InsertNextValue(cellId);
            }

            nextLoc = where;
          }
          else
          {
            ghostCellsPlease[processId]->InsertNextValue(0);
          }
        }
      }
      if ((gl == 1) && insideIds[i]) // points you have in my spatial region,
      {                              // which I may need ghost cells for
        for (j = 0; j < insideIds[i]->GetNumberOfTuples();)
        {
          gid = insideIds[i]->GetValue(j);
          int used = svtkPDistributedDataFilter::GlobalPointIdIsUsed(myGrid, gid, globalToLocalMap);
          if (used)
          {
            ghostCellsPlease[i]->InsertNextValue(gid);
            ghostCellsPlease[i]->InsertNextValue(0);
          }

          ncells = insideIds[i]->GetValue(j + 1);
          j += (ncells + 2);
        }
      }
    }

    if (gl > 1)
    {
      if (ghostPointIds[me]) // these points are actually inside my region
      {
        size = ghostPointIds[me]->GetNumberOfTuples();

        for (i = 0; i < size;)
        {
          gid = ghostPointIds[me]->GetValue(i);
          ncells = ghostPointIds[me]->GetValue(i + 1);

          mapIt = insidePointMap->IntMultiMap.find(gid);

          if (mapIt != insidePointMap->IntMultiMap.end())
          {
            while (mapIt->first == gid)
            {
              processId = mapIt->second;
              ghostCellsPlease[processId]->InsertNextValue(gid);
              ghostCellsPlease[processId]->InsertNextValue(ncells);

              for (k = 0; k < ncells; k++)
              {
                svtkIdType cellId = ghostPointIds[me]->GetValue(i + 1 + k);
                ghostCellsPlease[processId]->InsertNextValue(cellId);
              }

              ++mapIt;
            }
          }
          i += (ncells + 2);
        }
      }
    }

    this->FreeIntArrays(ghostPointIds);
    this->FreeIntArrays(insideIds);
    this->FreeIntArrays(processList);

    // Exchange these ghost cell requests.

    svtkIdTypeArray** ghostCellRequest = this->ExchangeIdArrays(ghostCellsPlease, DeleteYes, 0x001a);

    // Build a list of cell IDs satisfying each request received.
    // Delete request arrays.

    svtkIdList** sendCellList =
      this->BuildRequestedGrids(ghostCellRequest, myGrid, globalToLocalMap);

    // Build subgrids and exchange them

    svtkUnstructuredGrid* incomingGhostCells = this->ExchangeMergeSubGrids(
      sendCellList, DeleteYes, myGrid, DeleteNo, DuplicateCellsNo, GhostCellsYes, 0x001b);

    delete[] sendCellList;

    // Set ghost level of new cells, and merge into grid of other
    // ghost cells received.

    newGhostCellGrid =
      this->SetMergeGhostGrid(newGhostCellGrid, incomingGhostCells, gl, globalToLocalMap);

    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

    gl++;
  }

  delete insidePointMap;

  svtkUnstructuredGrid* newGrid = nullptr;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
  {
    svtkDataSet* grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(myGrid) ? 1 : 0);

    newGrid = svtkPDistributedDataFilter::MergeGrids(grids, 2, DeleteYes, useGlobalNodeIds, 0, 0);
  }
  else
  {
    newGrid = myGrid;
  }

  return newGrid;
}

//-----------------------------------------------------------------------
// We create an expanded grid that contains the ghost cells we need.
// This is in the case where IncludeAllIntersectingCells is ON.  This
// is easier in some respects because we know if that if a point lies
// in a region owned by a particular process, that process has all
// cells which use that point.  So it is easy to find ghost cells.
// On the otherhand, because cells are not uniquely assigned to regions,
// we may get multiple processes sending us the same cell, so we
// need to filter these out.
svtkUnstructuredGrid* svtkPDistributedDataFilter::AddGhostCellsDuplicateCellAssignment(
  svtkUnstructuredGrid* myGrid, svtkPDistributedDataFilterSTLCloak* globalToLocalMap)
{
  TimeLog timer("AddGhostCellsDuplicateCellAssignment", this->Timing);
  (void)timer;

  int i, j;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  svtkUnstructuredGrid* newGhostCellGrid = nullptr;
  svtkIdTypeArray** ghostPointIds = nullptr;
  svtkIdTypeArray** extraGhostPointIds = nullptr;

  std::map<int, int>::iterator mapIt;

  svtkPoints* pts = myGrid->GetPoints();

  while (gl <= this->GhostLevel)
  {
    // For ghost level 1, create a list for each process of points
    // in my grid which lie in that other process' spatial region.
    // This is normally all the points for which I need ghost cells,
    // with one EXCEPTION.  If a cell is axis-aligned, and a face of
    // the cell is on my upper boundary, then the vertices of this
    // face are in my spatial region, but I need their ghost cells.
    // I can detect this case when the process across the boundary
    // sends me a request for ghost cells of these points.
    //
    // For ghost level above 1, create a list for each process of
    // points in my ghost grid which are in that process' spatial
    // region and for which I need ghost cells.

    if (gl == 1)
    {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 1);
    }
    else
    {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
    }

    // Exchange these lists.

    svtkIdTypeArray** insideIds = this->ExchangeIdArrays(ghostPointIds, DeleteYes, 0x001c);

    // For ghost level 1, examine the points Ids I received from
    // other processes, to see if the exception described above
    // applies and I need ghost cells from them for those points.

    if (gl == 1)
    {
      svtkIdType* gidsCell = this->GetGlobalElementIds(myGrid);

      extraGhostPointIds = new svtkIdTypeArray*[nprocs];

      for (i = 0; i < nprocs; i++)
      {
        extraGhostPointIds[i] = nullptr;

        if (i == me)
        {
          continue;
        }

        if (insideIds[i] == nullptr)
        {
          continue;
        }

        svtkIdType size = insideIds[i]->GetNumberOfTuples();

        for (j = 0; j < size;)
        {
          svtkIdType gid = insideIds[i]->GetValue(j);
          svtkIdType ncells = insideIds[i]->GetValue(j + 1);
          j += (ncells + 2);

          mapIt = globalToLocalMap->IntMap.find(gid);

          if (mapIt == globalToLocalMap->IntMap.end())
          {
            // This point must be right on my boundary, and
            // not connected to any cell intersecting my region.

            continue;
          }
          svtkIdType localId = mapIt->second;

          double* pt = pts->GetPoint(localId);

          int interior = this->StrictlyInsideMyBounds(pt[0], pt[1], pt[2]);

          if (!interior)
          {
            extraGhostPointIds[i] =
              this->AddPointAndCells(gid, localId, myGrid, gidsCell, extraGhostPointIds[i]);
          }
        }
      }

      // Exchange these lists.

      svtkIdTypeArray** extraInsideIds =
        this->ExchangeIdArrays(extraGhostPointIds, DeleteYes, 0x001d);

      // Add the extra point ids to the previous list

      for (i = 0; i < nprocs; i++)
      {
        if (i == me)
        {
          continue;
        }

        if (extraInsideIds[i])
        {
          svtkIdType size = extraInsideIds[i]->GetNumberOfTuples();

          if (insideIds[i] == nullptr)
          {
            insideIds[i] = svtkIdTypeArray::New();
          }

          for (j = 0; j < size; j++)
          {
            insideIds[i]->InsertNextValue(extraInsideIds[i]->GetValue(j));
          }
        }
      }
      this->FreeIntArrays(extraInsideIds);
    }

    // Build a list of cell IDs satisfying each request received.

    svtkIdList** sendCellList = this->BuildRequestedGrids(insideIds, myGrid, globalToLocalMap);

    // Build subgrids and exchange them

    svtkUnstructuredGrid* incomingGhostCells = this->ExchangeMergeSubGrids(
      sendCellList, DeleteYes, myGrid, DeleteNo, DuplicateCellsYes, GhostCellsYes, 0x001e);

    delete[] sendCellList;

    // Set ghost level of new cells, and merge into grid of other
    // ghost cells received.

    newGhostCellGrid =
      this->SetMergeGhostGrid(newGhostCellGrid, incomingGhostCells, gl, globalToLocalMap);

    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

    gl++;
  }

  svtkUnstructuredGrid* newGrid = nullptr;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
  {
    svtkDataSet* grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(myGrid) ? 1 : 0);
    newGrid = svtkPDistributedDataFilter::MergeGrids(grids, 2, DeleteYes, useGlobalNodeIds, 0, 0);
  }
  else
  {
    newGrid = myGrid;
  }

  return newGrid;
}

//-----------------------------------------------------------------------
// For every process that sent me a list of point IDs, create a list
// of all the cells I have in my original grid containing those points.
// We omit cells the remote process already has.

svtkIdList** svtkPDistributedDataFilter::BuildRequestedGrids(svtkIdTypeArray** globalPtIds,
  svtkUnstructuredGrid* grid, svtkPDistributedDataFilterSTLCloak* ptIdMap)
{
  TimeLog timer("BuildRequestedGrids", this->Timing);
  (void)timer;

  svtkIdType id;
  int proc;
  int nprocs = this->NumProcesses;
  svtkIdType cellId;
  svtkIdType nelts;

  // for each process, create a list of the ids of cells I need
  // to send to it

  std::map<int, int>::iterator imap;

  svtkIdList* cellList = svtkIdList::New();

  svtkIdList** sendCells = new svtkIdList*[nprocs];

  for (proc = 0; proc < nprocs; proc++)
  {
    sendCells[proc] = svtkIdList::New();

    if (globalPtIds[proc] == nullptr)
    {
      continue;
    }

    if ((nelts = globalPtIds[proc]->GetNumberOfTuples()) == 0)
    {
      globalPtIds[proc]->Delete();
      continue;
    }

    svtkIdType* ptarray = globalPtIds[proc]->GetPointer(0);

    std::set<svtkIdType> subGridCellIds;

    svtkIdType nYourCells = 0;

    for (id = 0; id < nelts; id += (nYourCells + 2))
    {
      svtkIdType ptId = ptarray[id];
      nYourCells = ptarray[id + 1];

      imap = ptIdMap->IntMap.find(ptId);

      if (imap == ptIdMap->IntMap.end())
      {
        continue; // I don't have this point
      }

      svtkIdType myPtId = (svtkIdType)imap->second; // convert to my local point Id

      grid->GetPointCells(myPtId, cellList);

      svtkIdType nMyCells = cellList->GetNumberOfIds();

      if (nMyCells == 0)
      {
        continue;
      }

      if (nYourCells > 0)
      {
        // We don't send cells the remote process tells us it already
        // has.  This is much faster than removing duplicate cells on
        // the receive side.

        svtkIdType* remoteCells = ptarray + id + 2;
        svtkIdType* gidCells = this->GetGlobalElementIds(grid);

        svtkPDistributedDataFilter::RemoveRemoteCellsFromList(
          cellList, gidCells, remoteCells, nYourCells);
      }

      svtkIdType nSendCells = cellList->GetNumberOfIds();

      if (nSendCells == 0)
      {
        continue;
      }

      for (cellId = 0; cellId < nSendCells; cellId++)
      {
        subGridCellIds.insert(cellList->GetId(cellId));
      }
    }

    globalPtIds[proc]->Delete();

    svtkIdType numUniqueCellIds = static_cast<svtkIdType>(subGridCellIds.size());

    if (numUniqueCellIds == 0)
    {
      continue;
    }

    sendCells[proc]->SetNumberOfIds(numUniqueCellIds);
    svtkIdType next = 0;

    std::set<svtkIdType>::iterator it;

    for (it = subGridCellIds.begin(); it != subGridCellIds.end(); ++it)
    {
      sendCells[proc]->SetId(next++, *it);
    }
  }

  delete[] globalPtIds;

  cellList->Delete();

  return sendCells;
}

//-----------------------------------------------------------------------
void svtkPDistributedDataFilter::RemoveRemoteCellsFromList(
  svtkIdList* cellList, svtkIdType* gidCells, svtkIdType* remoteCells, svtkIdType nRemoteCells)
{
  svtkIdType id, nextId;
  svtkIdType id2;
  svtkIdType nLocalCells = cellList->GetNumberOfIds();

  // both lists should be very small, so we just do an n^2 lookup

  for (id = 0, nextId = 0; id < nLocalCells; id++)
  {
    svtkIdType localCellId = cellList->GetId(id);
    svtkIdType globalCellId = gidCells[localCellId];

    int found = 0;

    for (id2 = 0; id2 < nRemoteCells; id2++)
    {
      if (remoteCells[id2] == globalCellId)
      {
        found = 1;
        break;
      }
    }

    if (!found)
    {
      cellList->SetId(nextId++, localCellId);
    }
  }

  cellList->SetNumberOfIds(nextId);
}

//-----------------------------------------------------------------------
// Set the ghost levels for the points and cells in the received cells.
// Merge the new ghost cells into the supplied grid, and return the new grid.
// Delete all grids except the new merged grid.

svtkUnstructuredGrid* svtkPDistributedDataFilter::SetMergeGhostGrid(
  svtkUnstructuredGrid* ghostCellGrid, svtkUnstructuredGrid* incomingGhostCells, int ghostLevel,
  svtkPDistributedDataFilterSTLCloak* idMap)

{
  TimeLog timer("SetMergeGhostGrid", this->Timing);
  (void)timer;

  int i;

  if (incomingGhostCells->GetNumberOfCells() < 1)
  {
    if (incomingGhostCells != nullptr)
    {
      incomingGhostCells->Delete();
    }
    return ghostCellGrid;
  }

  // Set the ghost level of all new cells, and set the ghost level of all
  // the points.  We know some points in the new grids actually have ghost
  // level one lower, because they were on the boundary of the previous
  // grid.  This is OK if ghostLevel is > 1.  When we merge, svtkMergeCells
  // will skip these points because they are already in the previous grid.
  // But if ghostLevel is 1, those boundary points were in our original
  // grid, and we need to use the global ID map to determine if the
  // point ghost levels should be set to 0.

  svtkUnsignedCharArray* cellGL = incomingGhostCells->GetCellGhostArray();
  svtkUnsignedCharArray* ptGL = incomingGhostCells->GetPointGhostArray();

  unsigned char* ia = cellGL->GetPointer(0);

  for (i = 0; i < incomingGhostCells->GetNumberOfCells(); i++)
  {
    ia[i] = (unsigned char)ghostLevel;
  }

  ia = ptGL->GetPointer(0);

  for (i = 0; i < incomingGhostCells->GetNumberOfPoints(); i++)
  {
    ia[i] = (unsigned char)ghostLevel;
  }

  // now merge

  svtkUnstructuredGrid* mergedGrid = incomingGhostCells;

  if (ghostCellGrid && (ghostCellGrid->GetNumberOfCells() > 0))
  {
    svtkDataSet* sets[2];

    sets[0] = ghostCellGrid; // both sets will be deleted by MergeGrids
    sets[1] = incomingGhostCells;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(ghostCellGrid) ? 1 : 0);
    mergedGrid =
      svtkPDistributedDataFilter::MergeGrids(sets, 2, DeleteYes, useGlobalNodeIds, 0.0, 0);
  }

  // If this is ghost level 1, mark any points from our original grid
  // as ghost level 0.

  if (ghostLevel == 1)
  {
    ptGL = mergedGrid->GetPointGhostArray();

    svtkIdType* gidPoints = this->GetGlobalNodeIds(mergedGrid);
    int npoints = mergedGrid->GetNumberOfPoints();

    std::map<int, int>::iterator imap;

    for (i = 0; i < npoints; i++)
    {
      imap = idMap->IntMap.find(gidPoints[i]);

      if (imap != idMap->IntMap.end())
      {
        ptGL->SetValue(i, 0); // found among my ghost level 0 cells
      }
    }
  }

  return mergedGrid;
}

//-----------------------------------------------------------------------
svtkUnstructuredGrid* svtkPDistributedDataFilter::MergeGrids(svtkDataSet** sets, int nsets,
  int deleteDataSets, int useGlobalNodeIds, float pointMergeTolerance, int useGlobalCellIds)
{
  int i;

  if (nsets == 0)
  {
    return nullptr;
  }

  svtkUnstructuredGrid* newGrid = svtkUnstructuredGrid::New();
  // Any global ids should be consistent, so make sure they are passed.
  newGrid->GetPointData()->CopyGlobalIdsOn();
  newGrid->GetCellData()->CopyGlobalIdsOn();

  svtkMergeCells* mc = svtkMergeCells::New();
  mc->SetUnstructuredGrid(newGrid);

  mc->SetTotalNumberOfDataSets(nsets);

  svtkIdType totalPoints = 0;
  svtkIdType totalCells = 0;

  for (i = 0; i < nsets; i++)
  {
    totalPoints += sets[i]->GetNumberOfPoints();
    totalCells += sets[i]->GetNumberOfCells();
    // Only use global ids if they are available.
    useGlobalNodeIds = (useGlobalNodeIds && (sets[i]->GetPointData()->GetGlobalIds() != nullptr));
    useGlobalCellIds = (useGlobalCellIds && (sets[i]->GetCellData()->GetGlobalIds() != nullptr));
  }

  mc->SetTotalNumberOfPoints(totalPoints);
  mc->SetTotalNumberOfCells(totalCells);

  if (!useGlobalNodeIds)
  {
    mc->SetPointMergeTolerance(pointMergeTolerance);
  }
  mc->SetUseGlobalIds(useGlobalNodeIds);
  mc->SetUseGlobalCellIds(useGlobalCellIds);

  for (i = 0; i < nsets; i++)
  {
    mc->MergeDataSet(sets[i]);

    if (deleteDataSets)
    {
      sets[i]->Delete();
    }
  }

  mc->Finish();
  mc->Delete();

  return newGrid;
}

//-------------------------------------------------------------------------
void svtkPDistributedDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Kdtree: " << this->Kdtree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "MyId: " << this->MyId << endl;
  os << indent << "Target: " << this->Target << endl;
  os << indent << "Source: " << this->Source << endl;
  os << indent << "RetainKdtree: " << this->RetainKdtree << endl;
  os << indent << "IncludeAllIntersectingCells: " << this->IncludeAllIntersectingCells << endl;
  os << indent << "ClipCells: " << this->ClipCells << endl;

  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "UseMinimalMemory: " << this->UseMinimalMemory << endl;
}
