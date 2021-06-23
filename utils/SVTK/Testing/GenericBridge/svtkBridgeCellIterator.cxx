/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeCellIterator - Implementation of svtkGenericCellIterator.
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericCellIterator, svtkBridgeDataSet

#include "svtkBridgeCellIterator.h"

#include <cassert>

#include "svtkBridgeCell.h"
#include "svtkBridgeDataSet.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"

#include "svtkBridgeCellIteratorOnCellBoundaries.h"
#include "svtkBridgeCellIteratorOnCellList.h"
#include "svtkBridgeCellIteratorOnDataSet.h"
#include "svtkBridgeCellIteratorOne.h"

svtkStandardNewMacro(svtkBridgeCellIterator);

//-----------------------------------------------------------------------------
void svtkBridgeCellIterator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
svtkBridgeCellIterator::svtkBridgeCellIterator()
{
  //  this->DebugOn();
  this->CurrentIterator = nullptr;
  this->IteratorOnDataSet = svtkBridgeCellIteratorOnDataSet::New();
  this->IteratorOneCell = svtkBridgeCellIteratorOne::New();
  this->IteratorOnCellBoundaries = svtkBridgeCellIteratorOnCellBoundaries::New();
  this->IteratorOnCellList = svtkBridgeCellIteratorOnCellList::New();
}

//-----------------------------------------------------------------------------
svtkBridgeCellIterator::~svtkBridgeCellIterator()
{
  this->IteratorOnDataSet->Delete();
  this->IteratorOneCell->Delete();
  this->IteratorOnCellBoundaries->Delete();
  this->IteratorOnCellList->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgeCellIterator::Begin()
{
  if (this->CurrentIterator != nullptr)
  {
    this->CurrentIterator->Begin();
  }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
svtkTypeBool svtkBridgeCellIterator::IsAtEnd()
{
  int result = 1;

  if (this->CurrentIterator != nullptr)
  {
    result = this->CurrentIterator->IsAtEnd();
  }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Create an empty cell.
// \post result_exists: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIterator::NewCell()
{
  svtkGenericAdaptorCell* result = svtkBridgeCell::New();
  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void svtkBridgeCellIterator::GetCell(svtkGenericAdaptorCell* c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c != nullptr);

  this->CurrentIterator->GetCell(c);
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIterator::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());
  svtkGenericAdaptorCell* result = this->CurrentIterator->GetCell();
  assert("post: result_exits" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void svtkBridgeCellIterator::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->CurrentIterator->Next();
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over cells of `ds' of some dimension `dim'.
// \pre ds_exists: ds!=0
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
void svtkBridgeCellIterator::InitWithDataSet(svtkBridgeDataSet* ds, int dim)
{
  assert("pre: ds_exists" && ds != nullptr);
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 3));

  this->IteratorOnDataSet->InitWithDataSet(ds, dim);
  this->CurrentIterator = this->IteratorOnDataSet;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over boundary cells of `ds' of some dimension `dim'.
// \pre ds_exists: ds!=0
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
void svtkBridgeCellIterator::InitWithDataSetBoundaries(
  svtkBridgeDataSet* ds, int dim, int exterior_only)
{
  assert("pre: ds_exists" && ds != nullptr);
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 3));

  (void)ds;
  (void)dim;
  (void)exterior_only;

  assert("check: TODO" && 0);
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate on one cell `id' of `ds'.
// \pre ds_exists: ds!=0
// \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
void svtkBridgeCellIterator::InitWithOneCell(svtkBridgeDataSet* ds, svtkIdType cellid)
{
  assert("pre: ds_exists" && ds != nullptr);
  assert("pre: valid_id" && (cellid >= 0) && (cellid <= ds->GetNumberOfCells()));

  this->IteratorOneCell->InitWithOneCell(ds, cellid);
  this->CurrentIterator = this->IteratorOneCell;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on one cell `c'.
// \pre c_exists: c!=0
void svtkBridgeCellIterator::InitWithOneCell(svtkBridgeCell* c)
{
  assert("pre: c_exists" && c != nullptr);
  this->IteratorOneCell->InitWithOneCell(c);
  this->CurrentIterator = this->IteratorOneCell;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on boundary cells of a cell.
// \pre cell_exists: cell!=0
// \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
void svtkBridgeCellIterator::InitWithCellBoundaries(svtkBridgeCell* cell, int dim)
{
  assert("pre: cell_exists" && cell != nullptr);
  assert("pre: valid_dim_range" && ((dim == -1) || ((dim >= 0) && (dim < cell->GetDimension()))));
  this->IteratorOnCellBoundaries->InitWithCellBoundaries(cell, dim);
  this->CurrentIterator = this->IteratorOnCellBoundaries;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on neighbors defined by `cells' over the dataset `ds'.
// \pre cells_exist: cells!=0
// \pre ds_exists: ds!=0
void svtkBridgeCellIterator::InitWithCells(svtkIdList* cells, svtkBridgeDataSet* ds)
{
  assert("pre: cells_exist" && cells != nullptr);
  assert("pre: ds_exists" && ds != nullptr);

  this->IteratorOnCellList->InitWithCells(cells, ds);
  this->CurrentIterator = this->IteratorOnCellList;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on a boundary cell (defined by its points `pts' with coordinates
// `coords', dimension `dim' and unique id `cellid') of a cell.
// \pre coords_exist: coords!=0
// \pre pts_exist: pts!=0
// \pre valid_dim: dim>=0 && dim<=2
// \pre valid_points: pts->GetNumberOfIds()>dim
void svtkBridgeCellIterator::InitWithPoints(
  svtkPoints* coords, svtkIdList* pts, int dim, svtkIdType cellid)
{
  assert("pre: coords_exist" && coords != nullptr);
  assert("pre: pts_exist" && pts != nullptr);
  assert("pre: valid_dim" && dim >= 0 && dim <= 2);
  assert("pre: valid_points" && pts->GetNumberOfIds() > dim);

  this->IteratorOneCell->InitWithPoints(coords, pts, dim, cellid);
  this->CurrentIterator = this->IteratorOneCell;
}
