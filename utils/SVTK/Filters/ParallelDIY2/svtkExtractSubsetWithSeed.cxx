/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSubsetWithSeed.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSubsetWithSeed.h"

#include "svtkBoundingBox.h"
#include "svtkCell.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDIYExplicitAssigner.h"
#include "svtkDIYUtilities.h"
#include "svtkDataObjectTree.h"
#include "svtkExtractGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStaticCellLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredGrid.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

// clang-format off
#include "svtk_diy2.h"
// #define DIY_USE_SPDLOG
#include SVTK_DIY2(diy/mpi.hpp)
#include SVTK_DIY2(diy/master.hpp)
#include SVTK_DIY2(diy/link.hpp)
#include SVTK_DIY2(diy/reduce.hpp)
#include SVTK_DIY2(diy/reduce-operations.hpp)
#include SVTK_DIY2(diy/partners/swap.hpp)
#include SVTK_DIY2(diy/algorithms.hpp)
// clang-format on

#include <set>

template <typename T, int Size>
bool operator<(const svtkVector<T, Size>& lhs, const svtkVector<T, Size>& rhs)
{
  for (int cc = 0; cc < Size; ++cc)
  {
    if (lhs[cc] != rhs[cc])
    {
      return lhs[cc] < rhs[cc];
    }
  }
  return false;
}

namespace
{

struct BlockT
{
  svtkSmartPointer<svtkStructuredGrid> Input;
  svtkNew<svtkStaticCellLocator> CellLocator;
  std::set<svtkVector<int, 6> > Regions;

  // used to debugging, empty otherwise.
  std::vector<svtkSmartPointer<svtkDataSet> > Seeds;

  // these are generated in GenerateExtracts.
  std::vector<svtkSmartPointer<svtkDataSet> > Extracts;

  void GenerateExtracts();

  void AddExtracts(svtkPartitionedDataSet* pds)
  {
    if (!this->Input)
    {
      return;
    }

    unsigned int idx = pds->GetNumberOfPartitions();
    for (auto& extract : this->Extracts)
    {
      pds->SetPartition(idx++, extract);
    }
    for (auto& seed : this->Seeds)
    {
      pds->SetPartition(idx++, seed);
    }
  }
};

void BlockT::GenerateExtracts()
{
  if (!this->Input)
  {
    return;
  }

  this->Extracts.clear();
  svtkNew<svtkExtractGrid> extractor; // TODO: avoid creating new one on each iteration.
  for (const auto& voi : this->Regions)
  {
    extractor->SetInputDataObject(this->Input);
    extractor->SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
    extractor->Update();

    svtkStructuredGrid* clone = svtkStructuredGrid::New();
    clone->ShallowCopy(extractor->GetOutputDataObject(0));
    this->Extracts.push_back(clone);
    clone->FastDelete();
  }
}

using OrientationT = svtkVector3d;
using SeedT = std::tuple<svtkVector3d, svtkVector3d, svtkVector3d>;

svtkVector<int, 6> ComputeVOI(
  svtkStructuredGrid* input, const int ijk[3], const int propagation_mask[3])
{
  svtkVector<int, 6> voi;
  int data_ext[6];
  input->GetExtent(data_ext);
  for (int cc = 0; cc < 3; ++cc)
  {
    if (propagation_mask[cc] == 0)
    {
      voi[2 * cc] = ijk[cc];
      voi[2 * cc + 1] = ijk[cc] + 1;
    }
    else
    {
      voi[2 * cc] = data_ext[2 * cc];
      voi[2 * cc + 1] = data_ext[2 * cc + 1];
    }
  }

  return voi;
}

/**
 * Returns 3 unit vectors that identify the i,j,k directions for the cell.
 * Assumes svtkCell is svtkHexahedron.
 */
svtkVector3<svtkVector3d> GetCellOrientationVectors(svtkCell* cell)
{
  assert(cell->GetCellType() == SVTK_HEXAHEDRON);
  const std::pair<int, int> indexes[] = { std::make_pair(0, 1), std::make_pair(0, 3),
    std::make_pair(0, 4) };

  svtkVector3<svtkVector3d> values;
  for (int axis = 0; axis < 3; axis++)
  {
    const auto& idx = indexes[axis];
    svtkVector3d p0, p1;
    cell->GetPoints()->GetPoint(idx.first, p0.GetData());
    cell->GetPoints()->GetPoint(idx.second, p1.GetData());
    values[axis] = (p1 - p0);
    values[axis].Normalize();
  }
  return values;
}

std::pair<svtkVector3d, svtkVector3d> GetPropagationVectors(
  svtkCell* cell, const int progation_mask[3])
{
  const auto cell_orientation = GetCellOrientationVectors(cell);
  svtkVector3d values[3];
  int v_idx = 0;
  for (int axis = 0; axis < 3; axis++)
  {
    values[axis] = svtkVector3d(0.0);
    if (progation_mask[axis] > 0)
    {
      assert(v_idx < 2);
      values[v_idx++] = cell_orientation(axis);
    }
  }
  return std::make_pair(values[0], values[1]);
}

svtkVector3d GetFaceCenter(svtkCell* cell, int face_id)
{
  double weights[8];
  auto face = cell->GetFace(face_id);
  double center[3], pcoords[3];
  int subId = face->GetParametricCenter(pcoords);
  face->EvaluateLocation(subId, pcoords, center, weights);
  return svtkVector3d(center);
}

std::vector<SeedT> ExtractSliceFromSeed(const svtkVector3d& seed,
  const std::vector<svtkVector3d>& dirs, BlockT* b, const diy::Master::ProxyWithLink&)
{
  auto sg = b->Input;
  assert(svtkStructuredData::GetDataDescriptionFromExtent(sg->GetExtent()) == SVTK_XYZ_GRID);

  const svtkIdType cellid = b->CellLocator->FindCell(const_cast<double*>(seed.GetData()));
  if (cellid <= 0)
  {
    return std::vector<SeedT>();
  }

  // okay, we've determined that the seed lies in this block's grid. now we need to determine the
  // voi to extract based on the propagation directions provided.

  // using the cell's orientation, first determine which ijk axes the propagation directions
  // correspond to.
  auto cell_vectors = ::GetCellOrientationVectors(b->Input->GetCell(cellid));
  int propagation_mask[3] = { 0, 0, 0 };
  for (const auto& dir : dirs)
  {
    assert(dir.SquaredNorm() != 0);

    double max = 0.0;
    int axis = -1;
    for (int cc = 0; cc < 3; ++cc)
    {
      const auto dot = std::abs(dir.Dot(cell_vectors[cc]));
      if (dot > max)
      {
        max = dot;
        axis = cc;
      }
    }
    if (axis != -1)
    {
      propagation_mask[axis] = 1;
    }
  }
  assert(std::accumulate(propagation_mask, propagation_mask + 3, 0) < 3);

  int ijk[3];
  svtkStructuredData::ComputeCellStructuredCoordsForExtent(cellid, sg->GetExtent(), ijk);

  const auto voi = ComputeVOI(sg, ijk, propagation_mask);
  if (b->Regions.find(voi) != b->Regions.end())
  {
    return std::vector<SeedT>();
  }

  b->Regions.insert(voi);

  svtkVector<int, 6> cell_voi;
  svtkStructuredData::GetCellExtentFromPointExtent(voi.GetData(), cell_voi.GetData());

  std::vector<SeedT> next_seeds;
  for (int axis = 0; axis < 3; ++axis)
  {
    if (propagation_mask[axis] == 0)
    {
      continue;
    }

    // generate new seeds along each of the propagation axis e.g for i axis,
    // we add seeds along the j-k plane for min and max i values.

    // get the other two axes
    int dir_ii = (axis + 1) % 3;
    int dir_jj = (axis + 2) % 3;

    for (int iter = 0; iter < 2; ++iter)
    {
      ijk[axis] = cell_voi[2 * axis + iter];
      for (int ii = cell_voi[2 * dir_ii]; ii <= cell_voi[2 * dir_ii + 1]; ++ii)
      {
        for (int jj = cell_voi[2 * dir_jj]; jj <= cell_voi[2 * dir_jj + 1]; ++jj)
        {
          ijk[dir_ii] = ii;
          ijk[dir_jj] = jj;

          const auto acellid = svtkStructuredData::ComputeCellIdForExtent(sg->GetExtent(), ijk);
          auto cell = sg->GetCell(acellid);
          switch (sg->GetCellType(acellid))
          {
            case SVTK_HEXAHEDRON:
            {
              const auto new_seed = ::GetFaceCenter(cell, 2 * axis + iter);
              const auto pvecs = ::GetPropagationVectors(cell, propagation_mask);
              next_seeds.push_back(std::make_tuple(new_seed, pvecs.first, pvecs.second));
            }
            break;

            default:
              continue;
          }
        }
      }
    }
  }

#if 0
  svtkNew<svtkPoints> pts;
  pts->SetNumberOfPoints(next_seeds.size());
  for (size_t cc=0; cc < next_seeds.size(); ++cc)
  {
    pts->SetPoint(cc, std::get<0>(next_seeds[cc]).GetData());
  }

  svtkNew<svtkPolyData> pd;
  pd->SetPoints(pts);
  b->Seeds.push_back(pd);
#endif

  return next_seeds;
}

void Append(svtkPartitionedDataSet* input, svtkPartitionedDataSet* output)
{
  unsigned int next = output->GetNumberOfPartitions();
  output->SetNumberOfPartitions(next + input->GetNumberOfPartitions());
  for (unsigned int cc = 0, max = input->GetNumberOfPartitions(); cc < max; ++cc, ++next)
  {
    output->SetPartition(next, input->GetPartition(cc));
  }
}

template <typename V, typename SizeT>
svtkPartitionedDataSet* svtkSafeGet(V& vec, SizeT off)
{
  return (off < static_cast<SizeT>(vec.size())) ? vec[off] : nullptr;
}

void GenerateOutput(svtkMultiBlockDataSet* input, svtkMultiBlockDataSet* output,
  const std::vector<svtkSmartPointer<svtkPartitionedDataSet> >& parts, unsigned int& flat_index)
{
  auto max = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(max);
  for (unsigned int cc = 0; cc < max; ++cc)
  {
    ++flat_index;
    if (svtkSafeGet(parts, flat_index) == nullptr)
    {
      if (auto imb = svtkMultiBlockDataSet::SafeDownCast(input->GetBlock(cc)))
      {
        auto omb = svtkMultiBlockDataSet::New();
        output->SetBlock(cc, omb);
        omb->FastDelete();
        ::GenerateOutput(imb, omb, parts, flat_index);
      }
      else if (auto imp = svtkMultiPieceDataSet::SafeDownCast(input->GetBlock(cc)))
      {
        svtkNew<svtkPartitionedDataSet> pds;
        for (unsigned int piece = 0; piece < imp->GetNumberOfPieces(); ++piece)
        {
          ++flat_index;
          if (auto inpart = svtkSafeGet(parts, flat_index))
          {
            Append(inpart, pds);
          }
        }

        if (pds->GetNumberOfPartitions() > 0)
        {
          output->SetBlock(cc, pds);
        }
        else
        {
          auto omp = svtkMultiPieceDataSet::New();
          omp->SetNumberOfPieces(imp->GetNumberOfPieces());
          output->SetBlock(cc, omp);
          omp->FastDelete();
        }
      }
    }
    else
    {
      output->SetBlock(cc, svtkSafeGet(parts, flat_index));
    }

    if (input->HasMetaData(cc))
    {
      output->GetMetaData(cc)->Copy(input->GetMetaData(cc));
    }
  }
}

} // namespace {}

svtkStandardNewMacro(svtkExtractSubsetWithSeed);
svtkCxxSetObjectMacro(svtkExtractSubsetWithSeed, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkExtractSubsetWithSeed::svtkExtractSubsetWithSeed()
{
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkExtractSubsetWithSeed::~svtkExtractSubsetWithSeed()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void svtkExtractSubsetWithSeed::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "Direction: ";
  switch (this->Direction)
  {
    case LINE_I:
      os << "LINE_I" << endl;
      break;
    case LINE_J:
      os << "LINE_J" << endl;
      break;
    case LINE_K:
      os << "LINE_K" << endl;
      break;
    case PLANE_IJ:
      os << "PLANE_IJ" << endl;
      break;
    case PLANE_JK:
      os << "PLANE_JK" << endl;
      break;
    case PLANE_KI:
      os << "PLANE_KI" << endl;
      break;
    default:
      os << "(UNKNOWN)" << endl;
      break;
  }
}

//----------------------------------------------------------------------------
int svtkExtractSubsetWithSeed::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto inputDO = svtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = svtkDataObject::GetData(outputVector, 0);

  svtkSmartPointer<svtkDataObject> newoutput;
  if (svtkStructuredGrid::SafeDownCast(inputDO))
  {
    if (!svtkPartitionedDataSet::SafeDownCast(outputDO))
    {
      newoutput.TakeReference(svtkPartitionedDataSet::New());
    }
  }
  else if (auto inDOT = svtkDataObjectTree::SafeDownCast(inputDO))
  {
    if (outputDO == nullptr || !outputDO->IsA(inDOT->GetClassName()))
    {
      newoutput.TakeReference(inDOT->NewInstance());
    }
  }

  if (newoutput)
  {
    outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newoutput);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSubsetWithSeed::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto input = svtkDataObject::GetData(inputVector[0], 0);

  auto datasets = svtkDIYUtilities::GetDataSets(input);
  // prune non-structured grid datasets.
  auto prunePredicate = [](svtkDataObject* ds) {
    auto sg = svtkStructuredGrid::SafeDownCast(ds);
    // skip empty or non-3D grids.
    return sg == nullptr ||
      svtkStructuredData::GetDataDescriptionFromExtent(sg->GetExtent()) != SVTK_XYZ_GRID;
  };

  datasets.erase(std::remove_if(datasets.begin(), datasets.end(), prunePredicate), datasets.end());

  // since we're using collectives, if a rank has no blocks this can fall part
  // very quickly (see paraview/paraview#19391); hence we add a single block.
  if (datasets.size() == 0)
  {
    datasets.push_back(nullptr);
  }

  diy::mpi::communicator comm = svtkDIYUtilities::GetCommunicator(this->GetController());
  const int local_num_blocks = static_cast<int>(datasets.size());

  svtkDIYExplicitAssigner assigner(comm, local_num_blocks);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  svtkLogStartScope(TRACE, "populate master");
  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  assert(gids.size() == datasets.size());
  for (size_t lid = 0; lid < gids.size(); ++lid)
  {
    auto block = new BlockT();
    if (datasets[lid] != nullptr)
    {
      block->Input = svtkStructuredGrid::SafeDownCast(datasets[lid]);
      assert(block->Input != nullptr &&
        svtkStructuredData::GetDataDescriptionFromExtent(block->Input->GetExtent()) == SVTK_XYZ_GRID);
      block->CellLocator->SetDataSet(block->Input);
      block->CellLocator->BuildLocator();
    }
    master.add(gids[lid], block, new diy::Link);
  }
  svtkLogEndScope("populate master");

  // exchange bounding boxes to determine neighbours.
  svtkLogStartScope(TRACE, "populate block neighbours");
  std::map<int, std::vector<int> > neighbors;
  diy::all_to_all(master, assigner, [&neighbors](BlockT* b, const diy::ReduceProxy& rp) {
    svtkBoundingBox bbox;
    if (b->Input)
    {
      double bds[6];
      b->Input->GetBounds(bds);
      bbox.SetBounds(bds);
      bbox.Inflate(0.000001);
    }

    if (rp.round() == 0)
    {
      double bds[6];
      bbox.GetBounds(bds);
      for (int i = 0; i < rp.out_link().size(); ++i)
      {
        const auto dest = rp.out_link().target(i);
        rp.enqueue(dest, bds, 6);
      };
    }
    else
    {
      for (int i = 0; i < rp.in_link().size(); ++i)
      {
        const auto src = rp.in_link().target(i);
        double in_bds[6];
        rp.dequeue(src, in_bds, 6);
        svtkBoundingBox in_bbx(in_bds);
        if (src.gid != rp.gid() && in_bbx.IsValid() && bbox.IsValid() && in_bbx.Intersects(bbox))
        {
          svtkLogF(TRACE, "%d --> %d", rp.gid(), src.gid);
          neighbors[rp.gid()].push_back(src.gid);
        }
      }
    }
  });

  // update local links.
  for (auto& pair : neighbors)
  {
    auto l = new diy::Link();
    for (const auto& nid : pair.second)
    {
      l->add_neighbor(diy::BlockID(nid, assigner.rank(nid)));
    }
    master.replace_link(master.lid(pair.first), l);
  }
  svtkLogEndScope("populate block neighbours");

  int propagation_mask[3] = { 0, 0, 0 };
  switch (this->Direction)
  {
    case LINE_I:
      propagation_mask[0] = 1;
      break;
    case LINE_J:
      propagation_mask[1] = 1;
      break;
    case LINE_K:
      propagation_mask[2] = 1;
      break;
    case PLANE_IJ:
      propagation_mask[0] = 1;
      propagation_mask[1] = 1;
      break;
    case PLANE_JK:
      propagation_mask[1] = 1;
      propagation_mask[2] = 1;
      break;
    case PLANE_KI:
      propagation_mask[2] = 1;
      propagation_mask[0] = 1;
      break;
    default:
      break;
  }
  bool all_done = false;
  int round = 0;
  while (!all_done)
  {
    master.foreach ([this, &round, &propagation_mask](
                      BlockT* b, const diy::Master::ProxyWithLink& cp) {
      std::vector<SeedT> seeds;
      if (round == 0)
      {
        const svtkIdType cellid =
          (b->Input != nullptr) ? b->CellLocator->FindCell(this->GetSeed()) : -1;
        if (cellid >= 0)
        {
          auto p_vecs = ::GetPropagationVectors(b->Input->GetCell(cellid), propagation_mask);
          seeds.push_back(
            std::make_tuple(svtkVector3d(this->GetSeed()), p_vecs.first, p_vecs.second));
        }
      }
      else
      {
        // dequeue
        std::vector<int> incoming;
        cp.incoming(incoming);
        for (const int& gid : incoming)
        {
          if (cp.incoming(gid).size() > 0)
          {
            assert(b->Input != nullptr); // we should not be getting messages if we don't have data!
            std::vector<SeedT> next_seeds;
            cp.dequeue(gid, next_seeds);
            seeds.insert(seeds.end(), next_seeds.begin(), next_seeds.end());
          }
        }
      }

      std::vector<SeedT> next_seeds;
      for (const auto& seed : seeds)
      {
        std::vector<svtkVector3d> dirs;
        if (std::get<1>(seed).SquaredNorm() != 0)
        {
          dirs.push_back(std::get<1>(seed));
        }
        if (std::get<2>(seed).SquaredNorm() != 0)
        {
          dirs.push_back(std::get<2>(seed));
        }
        auto new_seeds = ::ExtractSliceFromSeed(std::get<0>(seed), dirs, b, cp);
        next_seeds.insert(next_seeds.end(), new_seeds.begin(), new_seeds.end());
      }

      if (next_seeds.size() > 0)
      {
        // enqueue
        for (const auto& neighbor : cp.link()->neighbors())
        {
          svtkLogF(
            TRACE, "r=%d: enqueing %d --> (%d, %d)", round, cp.gid(), neighbor.gid, neighbor.proc);
          cp.enqueue(neighbor, next_seeds);
        }
      }

      cp.collectives()->clear();

      const int has_seeds = (next_seeds.size() > 0) ? 1 : 0;
      cp.all_reduce(has_seeds, std::logical_or<int>());
    });
    svtkLogF(TRACE, "r=%d, exchange", round);
    master.exchange();
    all_done = (master.proxy(master.loaded_block()).read<int>() == 0);
    ++round;
  }

  // iterate over each block to combine the regions and extract.
  master.foreach ([&](BlockT* b, const diy::Master::ProxyWithLink&) { b->GenerateExtracts(); });

  //==========================================================================================
  // Pass extract to the output svtkDataObject
  //==========================================================================================
  // How data is passed to the output depends on the type of the dataset.
  if (auto outputPD = svtkPartitionedDataSet::GetData(outputVector, 0))
  {
    // Easiest case: we don't need to do anything special, just put out all
    // extracts as partitions. No need to take special care to match the
    // partition counts across ranks either.
    master.foreach (
      [&](BlockT* b, const diy::Master::ProxyWithLink&) { b->AddExtracts(outputPD); });
  }
  else if (auto outputPDC = svtkPartitionedDataSetCollection::GetData(outputVector, 0))
  {
    // Semi-easy case: ensure we create matching number of
    // svtkPartitionedDataSet's as in the input, but each can has as many
    // partitions as extracts. No need to take special care to match the
    // partitions across ranks.
    auto inputPDC = svtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
    assert(inputPDC);
    outputPDC->SetNumberOfPartitionedDataSets(inputPDC->GetNumberOfPartitionedDataSets());
    for (unsigned int cc = 0, max = inputPDC->GetNumberOfPartitionedDataSets(); cc < max; ++cc)
    {
      svtkNew<svtkPartitionedDataSet> pds;
      outputPDC->SetPartitionedDataSet(cc, pds);
      auto inputPDS = inputPDC->GetPartitionedDataSet(cc);
      for (unsigned int kk = 0, maxKK = inputPDS->GetNumberOfPartitions(); kk < maxKK; ++kk)
      {
        master.foreach ([&](BlockT* b, const diy::Master::ProxyWithLink&) {
          if (b->Input == inputPDS->GetPartition(kk))
          {
            b->AddExtracts(pds);
          }
        });
      }
    }
  }
  else if (auto outputMB = svtkMultiBlockDataSet::GetData(outputVector, 0))
  {
    auto inputMB = svtkMultiBlockDataSet::GetData(inputVector[0], 0);
    assert(inputMB != nullptr);
    // Worst case: we need to match up structure and across all ranks.

    std::vector<size_t> counts;
    int lid = 0;
    auto citer = inputMB->NewIterator();
    for (citer->InitTraversal();
         !citer->IsDoneWithTraversal() && lid < static_cast<int>(gids.size());
         citer->GoToNextItem())
    {
      auto b = master.block<BlockT>(lid);
      if (citer->GetCurrentDataObject() == b->Input)
      {
        counts.resize(citer->GetCurrentFlatIndex() + 1, 0);
        counts[citer->GetCurrentFlatIndex()] = b->Extracts.size() + b->Seeds.size();
        ++lid;
      }
    }

    size_t global_num_counts = 0;
    diy::mpi::all_reduce(comm, counts.size(), global_num_counts, diy::mpi::maximum<size_t>());
    counts.resize(global_num_counts, 0);

    std::vector<size_t> global_counts(global_num_counts);
    diy::mpi::all_reduce(comm, counts, global_counts, diy::mpi::maximum<size_t>());

    std::vector<svtkSmartPointer<svtkPartitionedDataSet> > parts(global_num_counts);
    lid = 0;
    citer->SkipEmptyNodesOff();
    for (citer->InitTraversal(); !citer->IsDoneWithTraversal(); citer->GoToNextItem())
    {
      const auto findex = citer->GetCurrentFlatIndex();
      if (findex >= static_cast<unsigned int>(global_num_counts))
      {
        // we're done.
        break;
      }
      const auto& count = global_counts[findex];
      if (count == 0)
      {
        if (!prunePredicate(citer->GetCurrentDataObject()))
        {
          ++lid;
        }
        continue;
      }
      else if (prunePredicate(citer->GetCurrentDataObject()) ||
        lid >= static_cast<int>(gids.size()))
      {
        // this block is not present locally (or was treated as such). So just
        // put a partitioned dataset with matching parts.
        svtkNew<svtkPartitionedDataSet> pds;
        pds->SetNumberOfPartitions(static_cast<unsigned int>(count));
        parts[findex] = pds;
      }
      else
      {
        auto b = master.block<BlockT>(lid);
        assert(b->Input == citer->GetCurrentDataObject());
        svtkNew<svtkPartitionedDataSet> pds;
        b->AddExtracts(pds);
        // pads will nullptr, if needed.
        pds->SetNumberOfPartitions(static_cast<unsigned int>(count));
        parts[findex] = pds;
        ++lid;
      }
    }
    citer->Delete();

    unsigned int flat_index = 0;
    ::GenerateOutput(inputMB, outputMB, parts, flat_index);
  }

  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Remove(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSubsetWithSeed::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPartitionedDataSetCollection");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPartitionedDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSubsetWithSeed::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Remove(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return 1;
}
