/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExpandMarkedElements.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExpandMarkedElements.h"

#include "svtkBoundingBox.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDIYExplicitAssigner.h"
#include "svtkDIYUtilities.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkLogger.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkSignedCharArray.h"
#include "svtkStaticPointLocator.h"
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

#include <algorithm>
#include <set>

namespace
{
void ShallowCopy(svtkDataObject* input, svtkDataObject* output)
{
  auto inCD = svtkCompositeDataSet::SafeDownCast(input);
  auto outCD = svtkCompositeDataSet::SafeDownCast(output);
  if (inCD && outCD)
  {
    outCD->CopyStructure(inCD);
    auto iter = inCD->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto clone = iter->GetCurrentDataObject()->NewInstance();
      clone->ShallowCopy(iter->GetCurrentDataObject());
      outCD->SetDataSet(iter, clone);
      clone->FastDelete();
    }
    iter->Delete();
  }
  else
  {
    output->ShallowCopy(input);
  }
}

struct BlockT
{
  svtkDataSet* Dataset = nullptr;
  svtkSmartPointer<svtkStaticPointLocator> Locator;
  svtkNew<svtkSignedCharArray> MarkedArray;
  svtkNew<svtkIntArray> UpdateFlags;
  std::vector<std::pair<diy::BlockID, svtkBoundingBox> > Neighbors;

  void BuildLocator();
  void EnqueueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp);
  void DequeueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp);

private:
  void Expand(int assoc, int round, const std::set<svtkIdType>& ptids);
  svtkNew<svtkIdList> CellIds;
  svtkNew<svtkIdList> PtIds;
};

void BlockT::BuildLocator()
{
  if (svtkPointSet::SafeDownCast(this->Dataset))
  {
    this->Locator = svtkSmartPointer<svtkStaticPointLocator>::New();
    this->Locator->SetTolerance(0.0);
    this->Locator->SetDataSet(this->Dataset);
    this->Locator->BuildLocator();
  }
}

void BlockT::EnqueueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp)
{
  const int rminusone = (round - 1);
  std::set<svtkIdType> chosen_ptids;
  if (assoc == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    for (svtkIdType cellid = 0, max = this->Dataset->GetNumberOfCells(); cellid < max; ++cellid)
    {
      if (this->MarkedArray->GetTypedComponent(cellid, 0) &&
        (this->UpdateFlags->GetTypedComponent(cellid, 0) == rminusone))
      {
        this->Dataset->GetCellPoints(cellid, this->PtIds);
        for (const svtkIdType& ptid : *this->PtIds)
        {
          chosen_ptids.insert(ptid);
        }
      }
    }
  }
  else
  {
    for (svtkIdType ptid = 0, max = this->Dataset->GetNumberOfPoints(); ptid < max; ++ptid)
    {
      if (this->MarkedArray->GetTypedComponent(ptid, 0) &&
        (this->UpdateFlags->GetTypedComponent(ptid, 0) == rminusone))
      {
        chosen_ptids.insert(ptid);
      }
    }
  }

  double pt[3];
  for (const svtkIdType& ptid : chosen_ptids)
  {
    this->Dataset->GetPoint(ptid, pt);
    for (const auto& pair : this->Neighbors)
    {
      if (pair.second.ContainsPoint(pt))
      {
        cp.enqueue(pair.first, pt, 3);
      }
    }
  }
  this->Expand(assoc, round, chosen_ptids);
}

void BlockT::DequeueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp)
{
  std::vector<int> incoming;
  cp.incoming(incoming);

  std::set<svtkIdType> pointIds;

  double pt[3];
  for (const int& gid : incoming)
  {
    while (cp.incoming(gid))
    {
      cp.dequeue(gid, pt, 3);
      double d;
      auto ptid = this->Locator ? this->Locator->FindClosestPointWithinRadius(0.000000000001, pt, d)
                                : this->Dataset->FindPoint(pt);
      if (ptid != -1)
      {
        pointIds.insert(ptid);
      }
    }
  }

  this->Expand(assoc, round, pointIds);
}

void BlockT::Expand(int assoc, int round, const std::set<svtkIdType>& ptids)
{
  if (assoc == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    for (const auto& startptid : ptids)
    {
      this->Dataset->GetPointCells(startptid, this->CellIds);
      for (const auto& cellid : *this->CellIds)
      {
        if (this->MarkedArray->GetTypedComponent(cellid, 0) == 0)
        {
          this->MarkedArray->SetTypedComponent(cellid, 0, 1);
          this->UpdateFlags->SetTypedComponent(cellid, 0, round);
        }
      }
    }
  }
  else
  {
    for (const auto& startptid : ptids)
    {
      if (this->MarkedArray->GetTypedComponent(startptid, 0) == 0)
      {
        this->MarkedArray->SetTypedComponent(startptid, 0, 1);
        this->UpdateFlags->SetTypedComponent(startptid, 0, round);
      }

      // get adjacent cells for the startptid.
      this->Dataset->GetPointCells(startptid, this->CellIds);
      for (const auto& cellid : *this->CellIds)
      {
        this->Dataset->GetCellPoints(cellid, this->PtIds);
        for (const svtkIdType& ptid : *this->PtIds)
        {
          if (this->MarkedArray->GetTypedComponent(ptid, 0) == 0)
          {
            this->MarkedArray->SetTypedComponent(ptid, 0, 1);
            this->UpdateFlags->SetTypedComponent(ptid, 0, round);
          }
        }
      }
    }
  }
}
}

svtkStandardNewMacro(svtkExpandMarkedElements);
svtkCxxSetObjectMacro(svtkExpandMarkedElements, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkExpandMarkedElements::svtkExpandMarkedElements()
{
  this->SetController(svtkMultiProcessController::GetGlobalController());
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkExpandMarkedElements::~svtkExpandMarkedElements()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkExpandMarkedElements::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto outputDO = svtkDataObject::GetData(outputVector, 0);
  ::ShallowCopy(svtkDataObject::GetData(inputVector[0], 0), outputDO);

  svtkInformation* info = this->GetInputArrayInformation(0);
  const int assoc = info->Get(svtkDataObject::FIELD_ASSOCIATION());
  auto datasets = svtkDIYUtilities::GetDataSets(outputDO);
  datasets.erase(std::remove_if(datasets.begin(), datasets.end(),
                   [](svtkDataSet* ds) { return (ds->GetNumberOfPoints() == 0); }),
    datasets.end());

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

  std::string arrayname;
  for (size_t lid = 0; lid < gids.size(); ++lid)
  {
    auto block = new BlockT();
    block->Dataset = datasets[lid];
    assert(block->Dataset != nullptr);
    const auto numElems = block->Dataset->GetNumberOfElements(assoc);
    if (auto array =
          svtkSignedCharArray::SafeDownCast(this->GetInputArrayToProcess(0, datasets[lid])))
    {
      // deep copy so we can modify it.
      block->MarkedArray->DeepCopy(array);
      if (arrayname.empty() && array->GetName())
      {
        arrayname = array->GetName();
      }
    }
    else
    {
      block->MarkedArray->SetNumberOfTuples(numElems);
      block->MarkedArray->FillValue(0);
    }
    assert(block->MarkedArray->GetNumberOfTuples() == numElems);
    block->UpdateFlags->SetNumberOfTuples(numElems);
    block->UpdateFlags->FillValue(-1);
    block->BuildLocator();

    master.add(gids[lid], block, new diy::Link);
  }
  svtkLogEndScope("populate master");

  // exchange bounding boxes to determine neighbours; helps avoid all_to_all
  // communication.
  svtkLogStartScope(TRACE, "populate block neighbours");
  diy::all_to_all(master, assigner, [](BlockT* b, const diy::ReduceProxy& rp) {
    double bds[6];
    b->Dataset->GetBounds(bds);
    const svtkBoundingBox bbox(bds);
    if (rp.round() == 0)
    {
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
        if (src.gid != rp.gid() && in_bbx.IsValid() && in_bbx.Intersects(bbox))
        {
          svtkLogF(TRACE, "%d --> %d", rp.gid(), src.gid);
          b->Neighbors.push_back(std::make_pair(src, in_bbx));
        }
      }
    }
  });

  // update local links.
  for (int cc = 0; cc < static_cast<int>(gids.size()); ++cc)
  {
    auto b = master.block<BlockT>(cc);
    if (b->Neighbors.size() > 0)
    {
      auto l = new diy::Link();
      for (const auto& npair : b->Neighbors)
      {
        l->add_neighbor(npair.first);
      }
      master.replace_link(cc, l);
    }
  }
  svtkLogEndScope("populate block neighbours");

  for (int round = 0; round < this->NumberOfLayers; ++round)
  {
    master.foreach ([&assoc, &round](BlockT* b, const diy::Master::ProxyWithLink& cp) {
      b->EnqueueAndExpand(assoc, round, cp);
    });
    master.exchange();
    master.foreach ([&assoc, &round](BlockT* b, const diy::Master::ProxyWithLink& cp) {
      b->DequeueAndExpand(assoc, round, cp);
    });
  }

  if (arrayname.empty())
  {
    arrayname = "MarkedElements";
  }
  master.foreach ([&assoc, &arrayname](BlockT* b, const diy::Master::ProxyWithLink&) {
    b->MarkedArray->SetName(arrayname.c_str());
    b->Dataset->GetAttributes(assoc)->AddArray(b->MarkedArray);
  });
  return 1;
}

//----------------------------------------------------------------------------
void svtkExpandMarkedElements::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
}
