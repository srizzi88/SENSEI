/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOnCellList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeCellIteratorOnCellList - Iterate over cells of a dataset.
// .SECTION See Also
// svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy

#include "svtkBridgeCellIteratorOnCellList.h"

#include <cassert>

#include "svtkBridgeCell.h"
#include "svtkBridgeDataSet.h"
#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkVertex.h"

svtkStandardNewMacro(svtkBridgeCellIteratorOnCellList);

//-----------------------------------------------------------------------------
void svtkBridgeCellIteratorOnCellList::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOnCellList::svtkBridgeCellIteratorOnCellList()
{
  this->DataSet = nullptr;
  this->Cells = nullptr;
  this->Cell = svtkBridgeCell::New();
  this->Id = 0;
  //  this->DebugOn();
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOnCellList::~svtkBridgeCellIteratorOnCellList()
{
  if (this->DataSet != nullptr)
  {
    this->DataSet->Delete();
    this->DataSet = nullptr;
  }

  if (this->Cells != nullptr)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  this->Cell->Delete();
  this->Cell = nullptr;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgeCellIteratorOnCellList::Begin()
{
  this->Id = 0; // first id of the current dimension
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
svtkTypeBool svtkBridgeCellIteratorOnCellList::IsAtEnd()
{
  return this->Id >= this->Cells->GetNumberOfIds();
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void svtkBridgeCellIteratorOnCellList::GetCell(svtkGenericAdaptorCell* c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c != nullptr);

  svtkBridgeCell* c2 = static_cast<svtkBridgeCell*>(c);
  c2->Init(this->DataSet, this->Cells->GetId(this->Id));
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIteratorOnCellList::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());

  this->Cell->Init(this->DataSet, this->Cells->GetId(this->Id));
  svtkGenericAdaptorCell* result = this->Cell;

  assert("post: result_exits" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void svtkBridgeCellIteratorOnCellList::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Id++; // next id of the current dimension
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeCell.
// Iterate on neighbors defined by `cells' over the dataset `ds'.
// \pre cells_exist: cells!=0
// \pre ds_exists: ds!=0
void svtkBridgeCellIteratorOnCellList::InitWithCells(svtkIdList* cells, svtkBridgeDataSet* ds)
{
  assert("pre: cells_exist" && cells != nullptr);
  assert("pre: ds_exists" && ds != nullptr);

  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, ds);
  svtkSetObjectBodyMacro(Cells, svtkIdList, cells);
}
