/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3HeavyDataHandler.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3HeavyDataHandler.h"

#include "svtkCompositeDataSet.h"
#include "svtkDataObject.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXdmf3ArrayKeeper.h"
#include "svtkXdmf3ArraySelection.h"
#include "svtkXdmf3DataSet.h"

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(XdmfCurvilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfDomain.hpp)
#include SVTKXDMF3_HEADER(XdmfGraph.hpp)
#include SVTKXDMF3_HEADER(XdmfGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollection.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollectionType.hpp)
#include SVTKXDMF3_HEADER(core/XdmfItem.hpp)
#include SVTKXDMF3_HEADER(XdmfRectilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfRegularGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfSet.hpp)
#include SVTKXDMF3_HEADER(XdmfUnstructuredGrid.hpp)
// clang-format on

#include <cassert>

//------------------------------------------------------------------------------
shared_ptr<svtkXdmf3HeavyDataHandler> svtkXdmf3HeavyDataHandler::New(svtkXdmf3ArraySelection* fs,
  svtkXdmf3ArraySelection* cs, svtkXdmf3ArraySelection* ps, svtkXdmf3ArraySelection* gc,
  svtkXdmf3ArraySelection* sc, unsigned int processor, unsigned int nprocessors, bool dt, double t,
  svtkXdmf3ArrayKeeper* keeper, bool asTime)
{
  shared_ptr<svtkXdmf3HeavyDataHandler> p(new svtkXdmf3HeavyDataHandler());
  p->FieldArrays = fs;
  p->CellArrays = cs;
  p->PointArrays = ps;
  p->GridsCache = gc;
  p->SetsCache = sc;
  p->Rank = processor;
  p->NumProcs = nprocessors;
  p->doTime = dt;
  p->time = t;
  p->Keeper = keeper;
  p->AsTime = asTime;
  return p;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::Populate(
  shared_ptr<XdmfDomain> item, svtkDataObject* toFill)
{
  assert(toFill);

  shared_ptr<XdmfDomain> group = shared_dynamic_cast<XdmfDomain>(item);

  shared_ptr<XdmfGridCollection> asGC = shared_dynamic_cast<XdmfGridCollection>(item);
  bool isDomain = asGC ? false : true;
  bool isTemporal = false;
  if (asGC)
  {
    if (asGC->getType() == XdmfGridCollectionType::Temporal())
    {
      isTemporal = true;
    }
  }

  // ignore groups that are not in timestep we were asked for
  // but be sure to return everything within them
  bool lastTime = this->doTime;
  if (this->doTime && !(isDomain || isTemporal))
  {
    if (asGC->getTime())
    {
      if (asGC->getTime()->getValue() != this->time)
      {
        // don't return MB that doesn't match the requested time
        return nullptr;
      }
      // inside a match, make sure we get everything underneath
      this->doTime = false;
    }
  }

  svtkMultiBlockDataSet* topB = svtkMultiBlockDataSet::SafeDownCast(toFill);
  svtkDataObject* result;
  unsigned int cnt = 0;
  unsigned int nGridCollections = group->getNumberGridCollections();

  for (unsigned int i = 0; i < nGridCollections; i++)
  {
    if (!this->AsTime)
    {
      if (isDomain && !this->ShouldRead(i, nGridCollections))
      {
        topB->SetBlock(cnt++, nullptr);
        continue;
      }
      svtkMultiBlockDataSet* child = svtkMultiBlockDataSet::New();
      // result = this->Populate(group->getGridCollection(i), child);
      shared_ptr<XdmfDomain> tempDomain =
        shared_dynamic_cast<XdmfDomain>(group->getGridCollection(i));
      result = this->Populate(tempDomain, child);
      topB->SetBlock(cnt++, result);
      child->Delete();
    }
    else
    {
      svtkMultiBlockDataSet* child = svtkMultiBlockDataSet::New();
      // result = this->Populate(group->getGridCollection(i), child);
      shared_ptr<XdmfDomain> tempDomain =
        shared_dynamic_cast<XdmfDomain>(group->getGridCollection(i));
      result = this->Populate(tempDomain, child);
      if (result)
      {
        topB->SetBlock(cnt++, result);
      }
      child->Delete();
    }
  }
  unsigned int nUnstructuredGrids = group->getNumberUnstructuredGrids();
  for (unsigned int i = 0; i < nUnstructuredGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i, nUnstructuredGrids))
    {
      topB->SetBlock(cnt++, nullptr);
      continue;
    }
    shared_ptr<XdmfUnstructuredGrid> cGrid = group->getUnstructuredGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    svtkDataObject* child;
    if (nSets > 0)
    {
      child = svtkMultiBlockDataSet::New();
    }
    else
    {
      child = svtkUnstructuredGrid::New();
    }
    result = this->Populate(group->getUnstructuredGrid(i), child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(svtkCompositeDataSet::NAME(), cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nRectilinearGrids = group->getNumberRectilinearGrids();
  for (unsigned int i = 0; i < nRectilinearGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i, nRectilinearGrids))
    {
      topB->SetBlock(cnt++, nullptr);
      continue;
    }
    shared_ptr<XdmfRectilinearGrid> cGrid = group->getRectilinearGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    svtkDataObject* child;
    if (nSets > 0)
    {
      child = svtkMultiBlockDataSet::New();
    }
    else
    {
      child = svtkRectilinearGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(svtkCompositeDataSet::NAME(), cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nCurvilinearGrids = group->getNumberCurvilinearGrids();
  for (unsigned int i = 0; i < nCurvilinearGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i, nCurvilinearGrids))
    {
      topB->SetBlock(cnt++, nullptr);
      continue;
    }
    shared_ptr<XdmfCurvilinearGrid> cGrid = group->getCurvilinearGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    svtkDataObject* child;
    if (nSets > 0)
    {
      child = svtkMultiBlockDataSet::New();
    }
    else
    {
      child = svtkStructuredGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(svtkCompositeDataSet::NAME(), cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nRegularGrids = group->getNumberRegularGrids();
  for (unsigned int i = 0; i < nRegularGrids; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i, nRegularGrids))
    {
      topB->SetBlock(cnt++, nullptr);
      continue;
    }
    shared_ptr<XdmfRegularGrid> cGrid = group->getRegularGrid(i);
    unsigned int nSets = cGrid->getNumberSets();
    svtkDataObject* child;
    if (nSets > 0)
    {
      child = svtkMultiBlockDataSet::New();
    }
    else
    {
      child = svtkUniformGrid::New();
    }
    result = this->Populate(cGrid, child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(svtkCompositeDataSet::NAME(), cGrid->getName().c_str());
      cnt++;
    }
    child->Delete();
  }
  unsigned int nGraphs = group->getNumberGraphs();
  for (unsigned int i = 0; i < nGraphs; i++)
  {
    if (this->AsTime && !isTemporal && !this->ShouldRead(i, nGraphs))
    {
      topB->SetBlock(cnt++, nullptr);
      continue;
    }
    svtkMutableDirectedGraph* child = svtkMutableDirectedGraph::New();
    result = this->Populate(group->getGraph(i), child);
    if (result)
    {
      topB->SetBlock(cnt, result);
      topB->GetMetaData(cnt)->Set(
        svtkCompositeDataSet::NAME(), group->getGraph(i)->getName().c_str());
      cnt++;
    }
    child->Delete();
  }

  if (lastTime)
  {
    // restore time search now that we've done the group contents
    this->doTime = true;
  }

  if (isTemporal && topB->GetNumberOfBlocks() == 1)
  {
    // temporal collection is just a place holder for its content
    return topB->GetBlock(0);
  }

  return topB;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::Populate(shared_ptr<XdmfGrid> item, svtkDataObject* toFill)
{
  assert(toFill);

  shared_ptr<XdmfUnstructuredGrid> unsGrid = shared_dynamic_cast<XdmfUnstructuredGrid>(item);
  if (unsGrid)
  {
    unsigned int nSets = unsGrid->getNumberSets();
    if (nSets > 0)
    {
      svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(toFill);
      svtkUnstructuredGrid* child = svtkUnstructuredGrid::New();
      mbds->SetBlock(0, this->MakeUnsGrid(unsGrid, child, this->Keeper));
      mbds->GetMetaData((unsigned int)0)
        ->Set(svtkCompositeDataSet::NAME(), unsGrid->getName().c_str());
      for (unsigned int i = 0; i < nSets; i++)
      {
        svtkUnstructuredGrid* sub = svtkUnstructuredGrid::New();
        mbds->SetBlock(i + 1, this->ExtractSet(i, unsGrid, child, sub, this->Keeper));
        mbds->GetMetaData(i + 1)->Set(
          svtkCompositeDataSet::NAME(), unsGrid->getSet(i)->getName().c_str());
        sub->Delete();
      }
      child->Delete();
      return mbds;
    }
    return this->MakeUnsGrid(unsGrid, svtkUnstructuredGrid::SafeDownCast(toFill), this->Keeper);
  }

  shared_ptr<XdmfRectilinearGrid> recGrid = shared_dynamic_cast<XdmfRectilinearGrid>(item);
  if (recGrid)
  {
    unsigned int nSets = recGrid->getNumberSets();
    if (nSets > 0)
    {
      svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(toFill);
      svtkRectilinearGrid* child = svtkRectilinearGrid::New();
      mbds->SetBlock(0, this->MakeRecGrid(recGrid, child, this->Keeper));
      mbds->GetMetaData((unsigned int)0)
        ->Set(svtkCompositeDataSet::NAME(), recGrid->getName().c_str());
      for (unsigned int i = 0; i < nSets; i++)
      {
        svtkUnstructuredGrid* sub = svtkUnstructuredGrid::New();
        mbds->SetBlock(i + 1, this->ExtractSet(i, recGrid, child, sub, this->Keeper));
        mbds->GetMetaData(i + 1)->Set(
          svtkCompositeDataSet::NAME(), recGrid->getSet(i)->getName().c_str());
        sub->Delete();
      }
      child->Delete();
      return mbds;
    }
    return this->MakeRecGrid(recGrid, svtkRectilinearGrid::SafeDownCast(toFill), this->Keeper);
  }

  shared_ptr<XdmfCurvilinearGrid> crvGrid = shared_dynamic_cast<XdmfCurvilinearGrid>(item);
  if (crvGrid)
  {
    unsigned int nSets = crvGrid->getNumberSets();
    if (nSets > 0)
    {
      svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(toFill);
      svtkStructuredGrid* child = svtkStructuredGrid::New();
      mbds->SetBlock(0, this->MakeCrvGrid(crvGrid, child, this->Keeper));
      mbds->GetMetaData((unsigned int)0)
        ->Set(svtkCompositeDataSet::NAME(), crvGrid->getName().c_str());
      for (unsigned int i = 0; i < nSets; i++)
      {
        svtkUnstructuredGrid* sub = svtkUnstructuredGrid::New();
        mbds->SetBlock(i + 1, this->ExtractSet(i, crvGrid, child, sub, this->Keeper));
        mbds->GetMetaData(i + 1)->Set(
          svtkCompositeDataSet::NAME(), crvGrid->getSet(i)->getName().c_str());
        sub->Delete();
      }
      child->Delete();
      return mbds;
    }
    return this->MakeCrvGrid(crvGrid, svtkStructuredGrid::SafeDownCast(toFill), this->Keeper);
  }

  shared_ptr<XdmfRegularGrid> regGrid = shared_dynamic_cast<XdmfRegularGrid>(item);
  if (regGrid)
  {
    unsigned int nSets = regGrid->getNumberSets();
    if (nSets > 0)
    {
      svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(toFill);
      svtkImageData* child = svtkImageData::New();
      mbds->SetBlock(0, this->MakeRegGrid(regGrid, child, this->Keeper));
      mbds->GetMetaData((unsigned int)0)
        ->Set(svtkCompositeDataSet::NAME(), regGrid->getName().c_str());
      for (unsigned int i = 0; i < nSets; i++)
      {
        svtkUnstructuredGrid* sub = svtkUnstructuredGrid::New();
        mbds->SetBlock(i + 1, this->ExtractSet(i, regGrid, child, sub, this->Keeper));
        mbds->GetMetaData(i + 1)->Set(
          svtkCompositeDataSet::NAME(), crvGrid->getSet(i)->getName().c_str());
        sub->Delete();
      }
      child->Delete();
      return mbds;
    }
    return this->MakeRegGrid(regGrid, svtkImageData::SafeDownCast(toFill), this->Keeper);
  }
  return nullptr; // already spit a warning out before this
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::Populate(shared_ptr<XdmfGraph> item, svtkDataObject* toFill)
{
  assert(toFill);

  shared_ptr<XdmfGraph> graph = shared_dynamic_cast<XdmfGraph>(item);
  if (graph)
  {
    return this->MakeGraph(graph, svtkMutableDirectedGraph::SafeDownCast(toFill), this->Keeper);
  }
  return nullptr; // already spit a warning out before this
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::ShouldRead(unsigned int piece, unsigned int npieces)
{
  if (this->NumProcs < 1)
  {
    // no parallel information given to us, assume serial
    return true;
  }
  if (npieces == 1)
  {
    return true;
  }
  if (npieces < this->NumProcs)
  {
    if (piece == this->Rank)
    {
      return true;
    }
    return false;
  }

#if 1
  unsigned int mystart = this->Rank * npieces / this->NumProcs;
  unsigned int myend = (this->Rank + 1) * npieces / this->NumProcs;
  if (piece >= mystart)
  {
    if (piece < myend || (this->Rank == this->NumProcs - 1))
    {
      return true;
    }
  }
  return false;
#else
  if ((piece % this->NumProcs) == this->Rank)
  {
    return true;
  }
  else
  {
    return false;
  }
#endif
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::GridEnabled(shared_ptr<XdmfGrid> grid)
{
  return this->GridsCache->ArrayIsEnabled(grid->getName().c_str());
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::GridEnabled(shared_ptr<XdmfGraph> graph)
{
  return this->GridsCache->ArrayIsEnabled(graph->getName().c_str());
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::SetEnabled(shared_ptr<XdmfSet> set)
{
  return this->SetsCache->ArrayIsEnabled(set->getName().c_str());
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::ForThisTime(shared_ptr<XdmfGrid> grid)
{
  return (!this->doTime || (grid->getTime() && grid->getTime()->getValue() == this->time));
}

//------------------------------------------------------------------------------
bool svtkXdmf3HeavyDataHandler::ForThisTime(shared_ptr<XdmfGraph> graph)
{
  return (!this->doTime || (graph->getTime() && graph->getTime()->getValue() == this->time));
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::MakeUnsGrid(
  shared_ptr<XdmfUnstructuredGrid> grid, svtkUnstructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfToSVTK(
      this->FieldArrays, this->CellArrays, this->PointArrays, grid.get(), dataSet, keeper);
    return dataSet;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::MakeRecGrid(
  shared_ptr<XdmfRectilinearGrid> grid, svtkRectilinearGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfToSVTK(
      this->FieldArrays, this->CellArrays, this->PointArrays, grid.get(), dataSet, keeper);
    return dataSet;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::MakeCrvGrid(
  shared_ptr<XdmfCurvilinearGrid> grid, svtkStructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfToSVTK(
      this->FieldArrays, this->CellArrays, this->PointArrays, grid.get(), dataSet, keeper);
    return dataSet;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::MakeRegGrid(
  shared_ptr<XdmfRegularGrid> grid, svtkImageData* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfToSVTK(
      this->FieldArrays, this->CellArrays, this->PointArrays, grid.get(), dataSet, keeper);
    return dataSet;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::MakeGraph(
  shared_ptr<XdmfGraph> grid, svtkMutableDirectedGraph* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (dataSet && this->GridEnabled(grid) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfToSVTK(
      this->FieldArrays, this->CellArrays, this->PointArrays, grid.get(), dataSet, keeper);
    return dataSet;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkXdmf3HeavyDataHandler::ExtractSet(unsigned int setnum, shared_ptr<XdmfGrid> grid,
  svtkDataSet* dataSet, svtkUnstructuredGrid* subSet, svtkXdmf3ArrayKeeper* keeper)
{
  shared_ptr<XdmfSet> set = grid->getSet(setnum);
  if (dataSet && subSet && SetEnabled(set) && this->ForThisTime(grid))
  {
    svtkXdmf3DataSet::XdmfSubsetToSVTK(grid.get(), setnum, dataSet, subSet, keeper);
    return subSet;
  }
  return nullptr;
}
