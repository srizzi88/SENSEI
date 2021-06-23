/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRedistributeDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRedistributeDataSetFilter.h"

#include "svtkAppendFilter.h"
#include "svtkCellData.h"
#include "svtkDIYKdTreeUtilities.h"
#include "svtkDIYUtilities.h"
#include "svtkExtractCells.h"
#include "svtkFieldData.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkKdNode.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPlane.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticCellLinks.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

namespace
{
static const char* CELL_OWNERSHIP_ARRAYNAME = "__RDSF_CELL_OWNERSHIP__";
static const char* GHOST_CELL_ARRAYNAME = "__RDSF_GHOST_CELLS__";
}

namespace detail
{
svtkBoundingBox GetBounds(svtkDataObject* dobj)
{
  if (auto pds = svtkPartitionedDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    pds->GetBounds(bds);
    return svtkBoundingBox(bds);
  }
  else if (auto mbds = svtkMultiBlockDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    mbds->GetBounds(bds);
    return svtkBoundingBox(bds);
  }
  else if (auto ds = svtkDataSet::SafeDownCast(dobj))
  {
    double bds[6];
    ds->GetBounds(bds);
    return svtkBoundingBox(bds);
  }
  return svtkBoundingBox();
}

/**
 * For each cell in the `dataset`, this function will return the cut-indexes for
 * the `cuts` provided that the cell belongs to. If `duplicate_boundary_cells` is
 * `true`, the for boundary cells, there will be multiple cut-indexes that the
 * cell may belong to. Otherwise, a cell can belong to at most 1 region.
 */
std::vector<std::vector<int> > GenerateCellRegions(
  svtkDataSet* dataset, const std::vector<svtkBoundingBox>& cuts, bool duplicate_boundary_cells)
{
  assert(dataset != nullptr && cuts.size() > 0 && dataset->GetNumberOfCells() > 0);

  auto ghostCells = svtkUnsignedCharArray::SafeDownCast(
    dataset->GetCellData()->GetArray(svtkDataSetAttributes::GhostArrayName()));

  std::vector<std::vector<int> > cellRegions(dataset->GetNumberOfCells());

  // call GetCell/GetCellBounds once to make it thread safe (see svtkDataSet::GetCell).
  svtkNew<svtkGenericCell> acell;
  dataset->GetCell(0, acell);
  double bds[6];
  dataset->GetCellBounds(0, bds);

  const auto numCells = dataset->GetNumberOfCells();
  if (duplicate_boundary_cells)
  {
    // svtkKdNode helps us do fast cell/cut intersections. So convert each cut to a
    // svtkKdNode.
    std::vector<svtkSmartPointer<svtkKdNode> > kdnodes;
    for (const auto& bbox : cuts)
    {
      auto kdnode = svtkSmartPointer<svtkKdNode>::New();
      kdnode->SetDim(-1); // leaf.

      double cut_bounds[6];
      bbox.GetBounds(cut_bounds);
      kdnode->SetBounds(cut_bounds);
      kdnodes.push_back(std::move(kdnode));
    }
    svtkSMPThreadLocalObject<svtkGenericCell> gcellLO;
    svtkSMPTools::For(0, numCells,
      [dataset, ghostCells, &kdnodes, &gcellLO, &cellRegions](svtkIdType first, svtkIdType last) {
        auto gcell = gcellLO.Local();
        std::vector<double> weights(dataset->GetMaxCellSize());
        for (svtkIdType cellId = first; cellId < last; ++cellId)
        {
          if (ghostCells != nullptr &&
            (ghostCells->GetTypedComponent(cellId, 0) & svtkDataSetAttributes::DUPLICATECELL) != 0)
          {
            // skip ghost cells, they will not be extracted since they will be
            // extracted on ranks where they are not marked as ghosts.
            continue;
          }
          dataset->GetCell(cellId, gcell);
          double cellBounds[6];
          dataset->GetCellBounds(cellId, cellBounds);
          for (int cutId = 0; cutId < static_cast<int>(kdnodes.size()); ++cutId)
          {
            if (kdnodes[cutId]->IntersectsCell(
                  gcell, /*useDataBounds*/ 0, /*cellRegion*/ -1, cellBounds))
            {
              cellRegions[cellId].push_back(cutId);
            }
          }
        }
      });
  }
  else
  {
    // simply assign to region contain the cell center.
    svtkSMPThreadLocalObject<svtkGenericCell> gcellLO;
    svtkSMPTools::For(0, numCells,
      [dataset, ghostCells, &cuts, &gcellLO, &cellRegions](svtkIdType first, svtkIdType last) {
        auto gcell = gcellLO.Local();
        std::vector<double> weights(dataset->GetMaxCellSize());
        for (svtkIdType cellId = first; cellId < last; ++cellId)
        {
          if (ghostCells != nullptr &&
            (ghostCells->GetTypedComponent(cellId, 0) & svtkDataSetAttributes::DUPLICATECELL) != 0)
          {
            // skip ghost cells, they will not be extracted since they will be
            // extracted on ranks where they are not marked as ghosts.
            continue;
          }
          dataset->GetCell(cellId, gcell);
          double pcenter[3], center[3];
          int subId = gcell->GetParametricCenter(pcenter);
          gcell->EvaluateLocation(subId, pcenter, center, &weights[0]);
          for (int cutId = 0; cutId < static_cast<int>(cuts.size()); ++cutId)
          {
            const auto& bbox = cuts[cutId];
            if (bbox.ContainsPoint(center))
            {
              cellRegions[cellId].push_back(cutId);
              assert(cellRegions[cellId].size() == 1);
              break;
            }
          }
        }
      });
  }

  return cellRegions;
}

/**
 * Clip the dataset by the provided plane using svtkmClip.
 */
svtkSmartPointer<svtkUnstructuredGrid> ClipPlane(svtkDataSet* dataset, svtkSmartPointer<svtkPlane> plane)
{
  if (!dataset)
    return nullptr;

  svtkNew<svtkTableBasedClipDataSet> clipper;
  clipper->SetInputDataObject(dataset);
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();
  clipper->Update();

  auto clipperOutput = svtkUnstructuredGrid::SafeDownCast(clipper->GetOutputDataObject(0));
  if (clipperOutput &&
    (clipperOutput->GetNumberOfCells() > 0 || clipperOutput->GetNumberOfPoints() > 0))
  {
    return clipperOutput;
  }
  return nullptr;
}

}

svtkStandardNewMacro(svtkRedistributeDataSetFilter);
svtkCxxSetObjectMacro(svtkRedistributeDataSetFilter, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkRedistributeDataSetFilter::svtkRedistributeDataSetFilter()
  : Controller(nullptr)
  , BoundaryMode(svtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION)
  , NumberOfPartitions(0)
  , PreservePartitionsInOutput(false)
  , GenerateGlobalCellIds(true)
  , UseExplicitCuts(false)
  , ExpandExplicitCuts(true)
  , EnableDebugging(false)
  , ValidDim{ true, true, true }
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkRedistributeDataSetFilter::~svtkRedistributeDataSetFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkRedistributeDataSetFilter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPartitionedDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::SetExplicitCuts(const std::vector<svtkBoundingBox>& boxes)
{
  if (this->ExplicitCuts != boxes)
  {
    this->ExplicitCuts = boxes;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::RemoveAllExplicitCuts()
{
  if (this->ExplicitCuts.size() > 0)
  {
    this->ExplicitCuts.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::AddExplicitCut(const svtkBoundingBox& bbox)
{
  if (bbox.IsValid() &&
    std::find(this->ExplicitCuts.begin(), this->ExplicitCuts.end(), bbox) ==
      this->ExplicitCuts.end())
  {
    this->ExplicitCuts.push_back(bbox);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::AddExplicitCut(const double bounds[6])
{
  svtkBoundingBox bbox(bounds);
  this->AddExplicitCut(bbox);
}

//----------------------------------------------------------------------------
int svtkRedistributeDataSetFilter::GetNumberOfExplicitCuts() const
{
  return static_cast<int>(this->ExplicitCuts.size());
}

//----------------------------------------------------------------------------
const svtkBoundingBox& svtkRedistributeDataSetFilter::GetExplicitCut(int index) const
{
  if (index >= 0 && index < this->GetNumberOfExplicitCuts())
  {
    return this->ExplicitCuts[index];
  }

  static svtkBoundingBox nullbox;
  return nullbox;
}

//----------------------------------------------------------------------------
int svtkRedistributeDataSetFilter::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto inputDO = svtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = svtkDataObject::GetData(outputVector, 0);
  auto outInfo = outputVector->GetInformationObject(0);

  if (svtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    if (!svtkMultiBlockDataSet::SafeDownCast(outputDO))
    {
      auto output = svtkMultiBlockDataSet::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (!this->PreservePartitionsInOutput && !svtkUnstructuredGrid::GetData(outputVector, 0))
  {
    auto output = svtkUnstructuredGrid::New();
    outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  else if (this->PreservePartitionsInOutput && !svtkPartitionedDataSet::GetData(outputVector, 0))
  {
    auto output = svtkPartitionedDataSet::New();
    outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkRedistributeDataSetFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto inputDO = svtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = svtkDataObject::GetData(outputVector, 0);
  this->MarkValidDimensions(inputDO);

  if (this->UseExplicitCuts && this->ExpandExplicitCuts)
  {
    auto bbox = detail::GetBounds(inputDO);
    if (bbox.IsValid())
    {
      bbox.Inflate(0.1 * bbox.GetDiagonalLength());
    }
    this->Cuts = svtkRedistributeDataSetFilter::ExpandCuts(this->ExplicitCuts, bbox);
  }
  else if (this->UseExplicitCuts)
  {
    this->Cuts = this->ExplicitCuts;
  }
  else
  {
    this->Cuts = this->GenerateCuts(inputDO);
  }
  this->UpdateProgress(0.25);

  if (auto inputMBDS = svtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    this->SetProgressShiftScale(0.25, 0.75);
    auto outputMBDS = svtkMultiBlockDataSet::SafeDownCast(outputDO);
    if (!outputMBDS)
    {
      svtkLogF(ERROR, "output should be a svtkMultiBlockDataSet");
      return 0;
    }
    svtkIdType mb_offset = 0;
    return this->RedistributeMultiBlockDataSet(inputMBDS, outputMBDS, &mb_offset);
  }

  svtkSmartPointer<svtkPartitionedDataSet> parts = svtkPartitionedDataSet::SafeDownCast(outputDO);
  if (!parts)
  {
    parts = svtkSmartPointer<svtkPartitionedDataSet>::New();
  }

  this->SetProgressShiftScale(0.25, 0.5);
  if (!this->Redistribute(inputDO, parts, this->Cuts))
  {
    return 0;
  }

  if (auto outputPDS = svtkPartitionedDataSet::SafeDownCast(outputDO))
  {
    if (!this->EnableDebugging)
    {
      // if output is svtkPartitionedDataSet, let's prune empty partitions. Not
      // necessary, but should help avoid people reading too much into the
      // partitions generated on each rank.
      outputPDS->RemoveNullPartitions();
    }
  }
  else if (auto outputUG = svtkUnstructuredGrid::SafeDownCast(outputDO))
  {
    svtkNew<svtkAppendFilter> appender;
    for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
    {
      if (auto ds = parts->GetPartition(cc))
      {
        appender->AddInputDataObject(ds);
      }
    }
    if (appender->GetNumberOfInputConnections(0) > 1)
    {
      appender->Update();
      outputUG->ShallowCopy(appender->GetOutputDataObject(0));
    }
    else if (appender->GetNumberOfInputConnections(0) == 1)
    {
      outputUG->ShallowCopy(appender->GetInputDataObject(0, 0));
    }
    outputUG->GetFieldData()->PassData(inputDO->GetFieldData());
  }
  this->SetProgressShiftScale(0.0, 1.0);
  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
std::vector<svtkBoundingBox> svtkRedistributeDataSetFilter::GenerateCuts(svtkDataObject* dobj)
{
  auto controller = this->GetController();
  const int num_partitions = (controller && this->GetNumberOfPartitions() == 0)
    ? controller->GetNumberOfProcesses()
    : this->GetNumberOfPartitions();
  return svtkDIYKdTreeUtilities::GenerateCuts(
    dobj, std::max(1, num_partitions), /*use_cell_centers=*/true, controller);
}

//----------------------------------------------------------------------------
bool svtkRedistributeDataSetFilter::Redistribute(svtkDataObject* inputDO,
  svtkPartitionedDataSet* outputPDS, const std::vector<svtkBoundingBox>& cuts,
  svtkIdType* mb_offset /*=nullptr*/)
{
  assert(outputPDS != nullptr);
  this->UpdateProgress(0.0);

  if (auto inputPDS = svtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    outputPDS->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));

    // assign global cell ids to inputDO, if not present.
    // we do this assignment before distributing cells if boundary mode is not
    // set to SPLIT_BOUNDARY_CELLS in which case we do after the split.
    svtkSmartPointer<svtkPartitionedDataSet> xfmedInput;
    if (this->GenerateGlobalCellIds && this->BoundaryMode != SPLIT_BOUNDARY_CELLS)
    {
      xfmedInput = this->AssignGlobalCellIds(inputPDS, mb_offset);
    }
    else
    {
      xfmedInput = inputPDS;
    }

    // We are distributing a svtkPartitionedDataSet. Our strategy is simple:
    // we split and distribute each input partition individually.
    // We then merge corresponding parts together to form the output partitioned
    // dataset.

    // since number of partitions need not match up across ranks, we do a quick
    // reduction to determine the number of iterations over partitions.
    // we limit to non-empty partitions.
    std::vector<svtkDataSet*> input_partitions;
    for (unsigned int cc = 0; cc < xfmedInput->GetNumberOfPartitions(); ++cc)
    {
      auto ds = xfmedInput->GetPartition(cc);
      if (ds && (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0))
      {
        input_partitions.push_back(ds);
      }
    }

    auto controller = this->GetController();
    if (controller && controller->GetNumberOfProcesses() > 1)
    {
      unsigned int mysize = static_cast<unsigned int>(input_partitions.size());
      unsigned int allsize = 0;
      controller->AllReduce(&mysize, &allsize, 1, svtkCommunicator::MAX_OP);
      assert(allsize >= mysize);
      input_partitions.resize(allsize, nullptr);
    }

    if (input_partitions.size() == 0)
    {
      // all ranks have empty data.
      return true;
    }

    std::vector<svtkSmartPointer<svtkPartitionedDataSet> > results;
    for (auto& ds : input_partitions)
    {
      svtkNew<svtkPartitionedDataSet> curOutput;
      if (this->RedistributeDataSet(ds, curOutput, cuts))
      {
        assert(curOutput->GetNumberOfPartitions() == static_cast<unsigned int>(cuts.size()));
        results.push_back(curOutput);
      }
    }

    // combine leaf nodes an all parts in the results to generate the output.
    for (unsigned int part = 0; part < outputPDS->GetNumberOfPartitions(); ++part)
    {
      svtkNew<svtkAppendFilter> appender;
      for (auto& pds : results)
      {
        if (auto ds = pds->GetPartition(part))
        {
          appender->AddInputDataObject(ds);
        }
      }
      if (appender->GetNumberOfInputConnections(0) == 1)
      {
        outputPDS->SetPartition(part, appender->GetInputDataObject(0, 0));
      }
      else if (appender->GetNumberOfInputConnections(0) > 1)
      {
        appender->Update();
        outputPDS->SetPartition(part, appender->GetOutputDataObject(0));
      }
    }
  }
  else if (auto inputDS = svtkDataSet::SafeDownCast(inputDO))
  {
    // assign global cell ids to inputDO, if not preset.
    // we do this assignment before distributing cells if boundary mode is not
    // set to SPLIT_BOUNDARY_CELLS in which case we do after the split.
    svtkSmartPointer<svtkDataSet> xfmedInput;
    if (this->GenerateGlobalCellIds && this->BoundaryMode != SPLIT_BOUNDARY_CELLS)
    {
      xfmedInput = this->AssignGlobalCellIds(inputDS, mb_offset);
    }
    else
    {
      xfmedInput = inputDS;
    }
    if (!this->RedistributeDataSet(xfmedInput, outputPDS, cuts))
    {
      return false;
    }
  }
  this->UpdateProgress(0.5);

  switch (this->GetBoundaryMode())
  {
    case svtkRedistributeDataSetFilter::SPLIT_BOUNDARY_CELLS:
      // by this point, boundary cells have been cloned on all boundary ranks.
      // locally, we will now simply clip each dataset by the corresponding
      // partition bounds.
      for (unsigned int cc = 0, max = outputPDS->GetNumberOfPartitions(); cc < max; ++cc)
      {
        if (auto ds = outputPDS->GetPartition(cc))
        {
          outputPDS->SetPartition(cc, this->ClipDataSet(ds, cuts[cc]));
        }
      }

      if (this->GenerateGlobalCellIds)
      {
        auto result = this->AssignGlobalCellIds(outputPDS, mb_offset);
        outputPDS->ShallowCopy(result);
      }
      break;

    case svtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION:
      // nothing to do, since we already assigned cells uniquely when splitting.
      break;

    case svtkRedistributeDataSetFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      // mark ghost cells using cell ownership information generated in
      // `SplitDataSet`.
      this->MarkGhostCells(outputPDS);
      break;

    default:
      // nothing to do.
      break;
  }
  this->UpdateProgress(0.75);

  if (!this->EnableDebugging)
  {
    // drop internal arrays
    for (unsigned int partId = 0, max = outputPDS->GetNumberOfPartitions(); partId < max; ++partId)
    {
      if (auto dataset = outputPDS->GetPartition(partId))
      {
        dataset->GetCellData()->RemoveArray(CELL_OWNERSHIP_ARRAYNAME);
        if (auto arr = dataset->GetCellData()->GetArray(GHOST_CELL_ARRAYNAME))
        {
          arr->SetName(svtkDataSetAttributes::GhostArrayName());
        }
      }
    }
  }
  this->UpdateProgress(1.0);

  return true;
}

//----------------------------------------------------------------------------
int svtkRedistributeDataSetFilter::RedistributeMultiBlockDataSet(
  svtkMultiBlockDataSet* input, svtkMultiBlockDataSet* output, svtkIdType* mb_offset /*=nullptr*/)
{
  if (!input || !output)
  {
    return 0;
  }

  output->CopyStructure(input);
  for (unsigned int block_id = 0; block_id < input->GetNumberOfBlocks(); ++block_id)
  {
    auto in_block = input->GetBlock(block_id);
    auto out_block = output->GetBlock(block_id);
    if (auto in_mbds = svtkMultiBlockDataSet::SafeDownCast(in_block))
    {
      auto out_mbds = svtkMultiBlockDataSet::SafeDownCast(out_block);
      this->RedistributeMultiBlockDataSet(in_mbds, out_mbds, mb_offset);
    }
    else if (auto in_mp = svtkMultiPieceDataSet::SafeDownCast(in_block))
    {
      auto out_mp = svtkMultiPieceDataSet::SafeDownCast(out_block);
      this->RedistributeMultiPieceDataSet(in_mp, out_mp, mb_offset);
    }
    else
    {
      // It's okay for inputDS to be null. Redistribute() should appropriately handle this.
      auto inputDS = svtkDataSet::SafeDownCast(in_block);
      svtkNew<svtkPartitionedDataSet> parts;

      if (!this->Redistribute(inputDS, parts, this->Cuts, mb_offset))
      {
        continue;
      }

      if (this->PreservePartitionsInOutput)
      {
        // have this block remain as a PDS
        output->SetBlock(block_id, parts);
      }
      else
      {
        // convert this block to an UG
        svtkNew<svtkAppendFilter> appender;
        for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
        {
          if (auto ds = parts->GetPartition(cc))
          {
            appender->AddInputDataObject(ds);
          }
        }
        if (appender->GetNumberOfInputConnections(0) > 0)
        {
          appender->Update();
          output->SetBlock(block_id, appender->GetOutput(0));
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// only for svtkMultiPieceDataSets that are part of a svtkMultiBlockDataSet
int svtkRedistributeDataSetFilter::RedistributeMultiPieceDataSet(
  svtkMultiPieceDataSet* input, svtkMultiPieceDataSet* output, svtkIdType* mb_offset /*=nullptr*/)
{
  if (!input || !output)
  {
    return 0;
  }

  output->CopyStructure(input);

  // because different ranks may have different numbers of pieces, we combine
  // into a single unstructured grid before redistributing data
  svtkNew<svtkAppendFilter> input_appender;
  svtkNew<svtkUnstructuredGrid> inputUG;
  for (unsigned int piece_id = 0; piece_id < input->GetNumberOfPieces(); ++piece_id)
  {
    if (auto ds = input->GetPiece(piece_id))
    {
      input_appender->AddInputDataObject(ds);
    }
  }
  if (input_appender->GetNumberOfInputConnections(0) > 0)
  {
    input_appender->Update();
    inputUG->ShallowCopy(input_appender->GetOutput(0));
  }

  svtkNew<svtkPartitionedDataSet> parts;

  if (!this->Redistribute(inputUG, parts, this->Cuts, mb_offset))
  {
    return 0;
  }

  if (this->PreservePartitionsInOutput)
  {
    // NOTE: Can't remove null partitions because it can result in errors
    // due to different ranks having different structures; only relevant
    // when we're working with svtkMultiBlockDataSets
    output->SetNumberOfPieces(parts->GetNumberOfPartitions());
    for (unsigned int piece_id = 0; piece_id < output->GetNumberOfPieces(); ++piece_id)
    {
      output->SetPiece(piece_id, parts->GetPartition(piece_id));
    }
  }
  else
  {
    // convert this block to an UG
    output->SetNumberOfPieces(1);
    svtkNew<svtkAppendFilter> appender;
    for (unsigned int cc = 0; cc < parts->GetNumberOfPartitions(); ++cc)
    {
      if (auto ds = parts->GetPartition(cc))
      {
        appender->AddInputDataObject(ds);
      }
    }
    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      output->SetPiece(0, appender->GetOutput(0));
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
bool svtkRedistributeDataSetFilter::RedistributeDataSet(
  svtkDataSet* inputDS, svtkPartitionedDataSet* outputPDS, const std::vector<svtkBoundingBox>& cuts)
{
  // note: inputDS can be null.
  auto parts = this->SplitDataSet(inputDS, cuts);
  assert(parts->GetNumberOfPartitions() == static_cast<unsigned int>(cuts.size()));

  auto pieces = svtkDIYKdTreeUtilities::Exchange(parts, this->GetController());
  assert(pieces->GetNumberOfPartitions() == parts->GetNumberOfPartitions());
  outputPDS->ShallowCopy(pieces);
  return true;
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkDataSet> svtkRedistributeDataSetFilter::ClipDataSet(
  svtkDataSet* dataset, const svtkBoundingBox& bbox)
{
  assert(dataset != nullptr);

  double bounds[6];
  bbox.GetBounds(bounds);
  svtkNew<svtkPlanes> box_planes;
  box_planes->SetBounds(bounds);

  svtkSmartPointer<svtkUnstructuredGrid> clipperOutput;
  for (int i = 0; i < box_planes->GetNumberOfPlanes(); ++i)
  {
    int dim = i / 2;
    // Only clip if this dimension in the original dataset's bounding box
    // (before redistribution) had a non-zero length, so we don't accidentally
    // clip away the full dataset.
    if (this->ValidDim[dim])
    {
      if (!clipperOutput)
      {
        clipperOutput = detail::ClipPlane(dataset, box_planes->GetPlane(i));
      }
      else
      {
        clipperOutput = detail::ClipPlane(clipperOutput, box_planes->GetPlane(i));
      }
    }
  }

  if (clipperOutput &&
    (clipperOutput->GetNumberOfCells() > 0 || clipperOutput->GetNumberOfPoints() > 0))
  {
    return clipperOutput;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkPartitionedDataSet> svtkRedistributeDataSetFilter::SplitDataSet(
  svtkDataSet* dataset, const std::vector<svtkBoundingBox>& cuts)
{
  if (!dataset || cuts.size() == 0 || dataset->GetNumberOfCells() == 0)
  //        (dataset->GetNumberOfCells() == 0 && dataset->GetNumberOfPoints() == 0))
  {
    svtkNew<svtkPartitionedDataSet> result;
    result->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));
    return result;
  }

  const auto numCells = dataset->GetNumberOfCells();

  // cell_regions tells us for each cell, which regions it belongs to.
  const bool duplicate_cells =
    this->GetBoundaryMode() != svtkRedistributeDataSetFilter::ASSIGN_TO_ONE_REGION;
  auto cell_regions = detail::GenerateCellRegions(dataset, cuts, duplicate_cells);
  assert(static_cast<svtkIdType>(cell_regions.size()) == numCells);

  // cell_ownership value is set to -1 is the cell doesn't belong to any cut
  // else it's set to the index of the cut in the cuts vector.
  svtkSmartPointer<svtkIntArray> cell_ownership;
  if (duplicate_cells)
  {
    // unless duplicating cells along boundary, no need to generate the
    // cell_ownership array. cell_ownership array is used to mark ghost cells
    // later on which don't exist if boundary cells are not duplicated.
    cell_ownership = svtkSmartPointer<svtkIntArray>::New();
    cell_ownership->SetName(CELL_OWNERSHIP_ARRAYNAME);
    cell_ownership->SetNumberOfComponents(1);
    cell_ownership->SetNumberOfTuples(numCells);
    cell_ownership->FillValue(-1);
  }

  // convert cell_regions to a collection of cell-ids for each region so that we
  // can use `svtkExtractCells` to extract cells for each region.
  std::vector<std::vector<svtkIdType> > region_cell_ids(cuts.size());
  svtkSMPTools::For(0, static_cast<int>(cuts.size()),
    [&region_cell_ids, &cell_regions, &numCells, &cell_ownership](int first, int last) {
      for (int cutId = first; cutId < last; ++cutId)
      {
        auto& cell_ids = region_cell_ids[cutId];
        for (svtkIdType cellId = 0; cellId < numCells; ++cellId)
        {
          const auto& cut_ids = cell_regions[cellId];
          auto iter = std::lower_bound(cut_ids.begin(), cut_ids.end(), cutId);
          if (iter != cut_ids.end() && *iter == cutId)
          {
            cell_ids.push_back(cellId);

            if (cell_ownership != nullptr && iter == cut_ids.begin())
            {
              // we treat the first cut number in the cut_ids vector as the
              // owner of the cell. `cell_ownership` array
              // will only be written to by that cut to avoid race condition
              // (note the svtkSMPTools::For()).

              // cell is owned by the numerically smaller cut.
              cell_ownership->SetTypedComponent(cellId, 0, cutId);
            }
          }
        }
      }
    });

  svtkNew<svtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(static_cast<unsigned int>(cuts.size()));

  // we create a clone of the input and add the
  // cell_ownership cell arrays to it so that they are propagated to each of the
  // extracted subsets and exchanged. It will be used later on to mark
  // ghost cells.
  auto clone = svtkSmartPointer<svtkDataSet>::Take(dataset->NewInstance());
  clone->ShallowCopy(dataset);
  clone->GetCellData()->AddArray(cell_ownership);

  svtkNew<svtkExtractCells> extractor;
  extractor->SetInputDataObject(clone);

  for (size_t region_idx = 0; region_idx < region_cell_ids.size(); ++region_idx)
  {
    const auto& cell_ids = region_cell_ids[region_idx];
    if (cell_ids.size() > 0)
    {
      extractor->SetCellIds(&cell_ids[0], static_cast<svtkIdType>(cell_ids.size()));
      extractor->Update();

      svtkNew<svtkUnstructuredGrid> ug;
      ug->ShallowCopy(extractor->GetOutputDataObject(0));
      result->SetPartition(static_cast<unsigned int>(region_idx), ug);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkDataSet> svtkRedistributeDataSetFilter::AssignGlobalCellIds(
  svtkDataSet* input, svtkIdType* mb_offset /*=nullptr*/)
{
  svtkNew<svtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(1);
  pds->SetPartition(0, input);
  auto output = this->AssignGlobalCellIds(pds, mb_offset);
  assert(output->GetNumberOfPartitions() == 1);
  return output->GetPartition(0);
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkPartitionedDataSet> svtkRedistributeDataSetFilter::AssignGlobalCellIds(
  svtkPartitionedDataSet* pieces, svtkIdType* mb_offset /*=nullptr*/)
{
  // if global cell ids are present everywhere, there's nothing to do!
  int missing_gids = 0;
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    svtkDataSet* dataset = pieces->GetPartition(partId);
    if (dataset && dataset->GetNumberOfCells() > 0 &&
      dataset->GetCellData()->GetGlobalIds() == nullptr)
    {
      missing_gids = 1;
      break;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    int any_missing_gids = 0;
    this->Controller->AllReduce(&missing_gids, &any_missing_gids, 1, svtkCommunicator::MAX_OP);
    missing_gids = any_missing_gids;
  }

  if (missing_gids == 0)
  {
    // input already has global cell ids.
    return pieces;
  }

  // We need to generate global cells ids since not all pieces (if any) have global cell
  // ids.
  svtkNew<svtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(pieces->GetNumberOfPartitions());
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    if (auto dataset = pieces->GetPartition(partId))
    {
      auto clone = dataset->NewInstance();
      clone->ShallowCopy(dataset);
      result->SetPartition(partId, clone);
      clone->FastDelete();
    }
  }

  svtkDIYKdTreeUtilities::GenerateGlobalCellIds(result, this->Controller, mb_offset);
  return result;
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::MarkGhostCells(svtkPartitionedDataSet* pieces)
{
  for (unsigned int partId = 0; partId < pieces->GetNumberOfPartitions(); ++partId)
  {
    svtkDataSet* dataset = pieces->GetPartition(partId);
    if (dataset == nullptr || dataset->GetNumberOfCells() == 0)
    {
      continue;
    }

    auto cell_ownership =
      svtkIntArray::SafeDownCast(dataset->GetCellData()->GetArray(CELL_OWNERSHIP_ARRAYNAME));
    if (!cell_ownership)
    {
      // cell_ownership is not generated if cells are being assigned uniquely to
      // parts since in that case there are no ghost cells.
      continue;
    }

    auto ghostCells = svtkUnsignedCharArray::SafeDownCast(
      dataset->GetCellData()->GetArray(svtkDataSetAttributes::GhostArrayName()));
    if (!ghostCells)
    {
      ghostCells = svtkUnsignedCharArray::New();
      // the array is renamed later on
      // ghostCells->SetName(svtkDataSetAttributes::GhostArrayName());
      ghostCells->SetName(GHOST_CELL_ARRAYNAME);
      ghostCells->SetNumberOfTuples(dataset->GetNumberOfCells());
      ghostCells->FillValue(0);
      dataset->GetCellData()->AddArray(ghostCells);
      ghostCells->FastDelete();
    }

    svtkSMPTools::For(0, dataset->GetNumberOfCells(), [&](svtkIdType start, svtkIdType end) {
      for (svtkIdType cc = start; cc < end; ++cc)
      {
        // any cell now owned by the current part is marked as a ghost cell.
        const auto cell_owner = cell_ownership->GetTypedComponent(cc, 0);
        auto gflag = ghostCells->GetTypedComponent(cc, 0);
        if (static_cast<int>(partId) == cell_owner)
        {
          gflag &= (~svtkDataSetAttributes::DUPLICATECELL);
        }
        else
        {
          gflag |= svtkDataSetAttributes::DUPLICATECELL;
        }
        ghostCells->SetTypedComponent(cc, 0, gflag);
      }
    });
  }
}

//----------------------------------------------------------------------------
std::vector<svtkBoundingBox> svtkRedistributeDataSetFilter::ExpandCuts(
  const std::vector<svtkBoundingBox>& cuts, const svtkBoundingBox& bounds)
{
  svtkBoundingBox cutsBounds;
  for (const auto& bbox : cuts)
  {
    cutsBounds.AddBox(bbox);
  }

  if (!bounds.IsValid() || !cutsBounds.IsValid() || cutsBounds.Contains(bounds))
  {
    // nothing to do.
    return cuts;
  }

  std::vector<svtkBoundingBox> result = cuts;
  for (auto& bbox : result)
  {
    if (!bbox.IsValid())
    {
      continue;
    }

    double bds[6];
    bbox.GetBounds(bds);
    for (int face = 0; face < 6; ++face)
    {
      if (bds[face] == cutsBounds.GetBound(face))
      {
        bds[face] = (face % 2 == 0) ? std::min(bds[face], bounds.GetBound(face))
                                    : std::max(bds[face], bounds.GetBound(face));
      }
    }
    bbox.SetBounds(bds);
    assert(bbox.IsValid()); // input valid implies output is valid too.
  }

  return result;
}

//----------------------------------------------------------------------------
// Determine which dimensions in the initial bounding box (before any inflation
// of the bounds occurs) has a non-zero length. This is necessary for clipping
// when the BoundaryMode is set to SPLIT_BOUNDARY_CELLS. Otherwise if a dataset
// ends up being 2D, performing plane clips on all sides of the bounding box may
// result in full dataset being clipped away.
void svtkRedistributeDataSetFilter::MarkValidDimensions(svtkDataObject* inputDO)
{
  static const int max_dim = 3;
  auto bbox = detail::GetBounds(inputDO);
  auto comm = svtkDIYUtilities::GetCommunicator(this->Controller);
  svtkDIYUtilities::AllReduce(comm, bbox);

  double len[max_dim];
  bbox.GetLengths(len);
  for (int i = 0; i < max_dim; ++i)
  {
    if (len[i] <= 0)
    {
      this->ValidDim[i] = false;
    }
    else
    {
      this->ValidDim[i] = true;
    }
  }
}

//----------------------------------------------------------------------------
void svtkRedistributeDataSetFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "BoundaryMode: " << this->BoundaryMode << endl;
  os << indent << "NumberOfPartitions: " << this->NumberOfPartitions << endl;
  os << indent << "PreservePartitionsInOutput: " << this->PreservePartitionsInOutput << endl;
  os << indent << "GenerateGlobalCellIds: " << this->GenerateGlobalCellIds << endl;
  os << indent << "UseExplicitCuts: " << this->UseExplicitCuts << endl;
  os << indent << "ExpandExplicitCuts: " << this->ExpandExplicitCuts << endl;
  os << indent << "EnableDebugging: " << this->EnableDebugging << endl;
}
