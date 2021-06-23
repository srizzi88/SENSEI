/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOne.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeCellIteratorOne - Iterate over cells of a dataset.
// .SECTION See Also
// svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy

#include "svtkBridgeCellIteratorOne.h"

#include <cassert>

#include "svtkBridgeCell.h"
#include "svtkBridgeDataSet.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"

#include "svtkLine.h"
#include "svtkPolyLine.h"
#include "svtkPolyVertex.h"
#include "svtkPolygon.h"
#include "svtkTriangle.h"
#include "svtkVertex.h"

svtkStandardNewMacro(svtkBridgeCellIteratorOne);

//-----------------------------------------------------------------------------
void svtkBridgeCellIteratorOne::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOne::svtkBridgeCellIteratorOne()
{
  this->DataSet = nullptr;
  this->InternalCell = nullptr;
  this->Cell = nullptr;
  this->Id = 0;
  this->cIsAtEnd = 0;
  //  this->DebugOn();
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOne::~svtkBridgeCellIteratorOne()
{
  if ((this->Cell != nullptr) && ((this->DataSet != nullptr) || (this->InternalCell != nullptr)))
  {
    // dataset mode or points mode
    this->Cell->Delete();
    this->Cell = nullptr;
  }
  if (this->DataSet != nullptr)
  {
    this->DataSet->Delete();
    this->DataSet = nullptr;
  }

  if (this->InternalCell != nullptr)
  {
    this->InternalCell->Delete();
    this->InternalCell = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgeCellIteratorOne::Begin()
{
  this->cIsAtEnd = 0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
svtkTypeBool svtkBridgeCellIteratorOne::IsAtEnd()
{
  return this->cIsAtEnd;
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void svtkBridgeCellIteratorOne::GetCell(svtkGenericAdaptorCell* c)
{
  assert("pre: not_at_end" && !this->IsAtEnd());
  assert("pre: c_exists" && c != nullptr);

  svtkBridgeCell* c2 = static_cast<svtkBridgeCell*>(c);
  if (this->DataSet != nullptr)
  {
    c2->Init(this->DataSet, this->Id);
  }
  else
  {
    if (this->InternalCell != nullptr)
    {
      c2->InitWithCell(this->InternalCell, this->Id);
    }
    else
    {
      c2->DeepCopy(this->Cell);
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIteratorOne::GetCell()
{
  assert("pre: not_at_end" && !this->IsAtEnd());

  svtkGenericAdaptorCell* result = this->Cell;

  assert("post: result_exits" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void svtkBridgeCellIteratorOne::Next()
{
  assert("pre: not_off" && !this->IsAtEnd());

  this->cIsAtEnd = 1;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate on one cell `id' of `ds'.
// \pre ds_exists: ds!=0
// \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
void svtkBridgeCellIteratorOne::InitWithOneCell(svtkBridgeDataSet* ds, svtkIdType cellid)
{
  assert("pre: ds_exists" && ds != nullptr);
  assert("pre: valid_id" && ((cellid >= 0) && (cellid <= ds->GetNumberOfCells())));

  if ((this->Cell != nullptr) && (this->DataSet == nullptr) && (this->InternalCell == nullptr))
  {
    // previous mode was InitWithOneCell(svtkBridgeCell *c)
    //    this->Cell->Delete();
    this->Cell = nullptr;
  }

  if (this->Cell == nullptr)
  {
    // first init or previous mode was InitWithOneCell(svtkBridgeCell *c)
    this->Cell = svtkBridgeCell::New();
  }

  svtkSetObjectBodyMacro(InternalCell, svtkCell, 0);
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, ds);
  this->Id = cellid;
  this->cIsAtEnd = 1;
  this->Cell->Init(this->DataSet, this->Id);
}
//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on one cell `c'.
// \pre c_exists: c!=0
void svtkBridgeCellIteratorOne::InitWithOneCell(svtkBridgeCell* c)
{
  assert("pre: c_exists" && c != nullptr);

  if ((this->Cell != nullptr) && ((this->DataSet != nullptr) || (this->InternalCell != nullptr)))
  {
    // dataset mode or points mode
    this->Cell->Delete();
  }
  svtkSetObjectBodyMacro(InternalCell, svtkCell, 0);
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, 0);

  this->Cell = c; // no register to prevent reference cycle with svtkBridgeCell
  this->Id = c->GetId();
  this->cIsAtEnd = 1;
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
void svtkBridgeCellIteratorOne::InitWithPoints(
  svtkPoints* coords, svtkIdList* pts, int dim, svtkIdType cellid)
{
  assert("pre: coords_exist" && coords != nullptr);
  assert("pre: pts_exist" && pts != nullptr);
  assert("pre: valid_dim" && dim >= 0 && dim <= 2);
  assert("pre: valid_points" && pts->GetNumberOfIds() > dim);

  if ((this->DataSet == nullptr) && (this->InternalCell == nullptr))
  {
    // previous mode was InitWithOneCell(svtkBridgeCell *c)
    //    this->Cell->Delete();
    this->Cell = nullptr;
  }

  if (this->Cell == nullptr)
  {
    // first init or previous mode was InitWithOneCell(svtkBridgeCell *c)
    this->Cell = svtkBridgeCell::New();
  }

  svtkCell* cell = nullptr;

  switch (dim)
  {
    case 2:
      if (pts->GetNumberOfIds() == 3)
      {
        cell = svtkTriangle::New();
      }
      else
      {
        cell = svtkPolygon::New();
      }
      break;
    case 1:
      // line or polyline
      if (pts->GetNumberOfIds() == 2)
      {
        cell = svtkLine::New();
      }
      else
      {
        cell = svtkPolyLine::New();
      }
      break;
    case 0:
      // vertex polyvertex
      if (pts->GetNumberOfIds() == 1)
      {
        cell = svtkVertex::New();
      }
      else
      {
        cell = svtkPolyVertex::New();
      }
      break;
  }
  cell->Points = coords;
  cell->PointIds = pts;
  svtkSetObjectBodyMacro(InternalCell, svtkCell, cell);
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, 0);
  this->Id = cellid;
  this->cIsAtEnd = 1;
  this->Cell->InitWithCell(this->InternalCell, this->Id);
}
