/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeCellIteratorOnDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeCellIteratorOnDataSet - Iterate over cells of a dataset.
// .SECTION See Also
// svtkBridgeCellIterator, svtkBridgeDataSet, svtkBridgeCellIteratorStrategy

#include "svtkBridgeCellIteratorOnDataSet.h"

#include <cassert>

#include "svtkBridgeCell.h"
#include "svtkBridgeDataSet.h"
#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkBridgeCellIteratorOnDataSet);

//-----------------------------------------------------------------------------
void svtkBridgeCellIteratorOnDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOnDataSet::svtkBridgeCellIteratorOnDataSet()
{
  this->DataSet = nullptr;
  this->Cell = svtkBridgeCell::New();
  this->Id = 0;
  this->Size = 0;
  //  this->DebugOn();
}

//-----------------------------------------------------------------------------
svtkBridgeCellIteratorOnDataSet::~svtkBridgeCellIteratorOnDataSet()
{
  if (this->DataSet != nullptr)
  {
    this->DataSet->Delete();
    this->DataSet = nullptr;
  }
  this->Cell->Delete();
  this->Cell = nullptr;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgeCellIteratorOnDataSet::Begin()
{
  this->Id = -1;
  this->Next(); // skip cells of other dimensions
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
svtkTypeBool svtkBridgeCellIteratorOnDataSet::IsAtEnd()
{
  return (this->Id >= this->Size);
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void svtkBridgeCellIteratorOnDataSet::GetCell(svtkGenericAdaptorCell* c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c != nullptr);

  svtkBridgeCell* c2 = static_cast<svtkBridgeCell*>(c);
  c2->Init(this->DataSet, this->Id);
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
svtkGenericAdaptorCell* svtkBridgeCellIteratorOnDataSet::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());

  this->Cell->Init(this->DataSet, this->Id);
  svtkGenericAdaptorCell* result = this->Cell;

  assert("post: result_exits" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void svtkBridgeCellIteratorOnDataSet::Next()
{
  assert("pre: not_off" && !IsAtEnd());

  svtkIdType size = this->Size;
  svtkCell* c;
  int found;

  this->Id++;

  if (this->Dim >= 0) // skip cells of other dimensions than this->Dim
  {
    found = 0;
    while ((this->Id < size) && (!found))
    {
      c = this->DataSet->Implementation->GetCell(this->Id);
      found = c->GetCellDimension() == this->Dim;
      this->Id++;
    }
    if (found)
    {
      this->Id--;
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over cells of `ds' of some dimension `dim'.
// \pre ds_exists: ds!=0
// \pre valid_dim_range: (dim>=-1) && (dim<=3)
void svtkBridgeCellIteratorOnDataSet::InitWithDataSet(svtkBridgeDataSet* ds, int dim)
{
  assert("pre: ds_exists" && ds != nullptr);
  assert("pre: valid_dim_range" && (dim >= -1) && (dim <= 3));

  this->Dim = dim;
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, ds);
  this->Size = ds->GetNumberOfCells();
  this->Id = this->Size; // at end
}
