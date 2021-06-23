/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgePointIterator - Implementation of svtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericPointIterator, svtkBridgeDataSet

#include "svtkBridgePointIterator.h"

#include <cassert>

#include "svtkBridgeDataSet.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"

#include "svtkBridgePointIteratorOnCell.h"
#include "svtkBridgePointIteratorOnDataSet.h"
#include "svtkBridgePointIteratorOne.h"

svtkStandardNewMacro(svtkBridgePointIterator);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
svtkBridgePointIterator::svtkBridgePointIterator()
{
  this->CurrentIterator = nullptr;
  this->IteratorOnDataSet = svtkBridgePointIteratorOnDataSet::New();
  this->IteratorOne = svtkBridgePointIteratorOne::New();
  this->IteratorOnCell = svtkBridgePointIteratorOnCell::New();
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
svtkBridgePointIterator::~svtkBridgePointIterator()
{
  this->IteratorOnDataSet->Delete();
  this->IteratorOne->Delete();
  this->IteratorOnCell->Delete();
}

//-----------------------------------------------------------------------------
void svtkBridgePointIterator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgePointIterator::Begin()
{
  if (this->CurrentIterator != nullptr)
  {
    this->CurrentIterator->Begin();
  }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
svtkTypeBool svtkBridgePointIterator::IsAtEnd()
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
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void svtkBridgePointIterator::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->CurrentIterator->Next();
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double* svtkBridgePointIterator::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());

  double* result = this->CurrentIterator->GetPosition();

  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void svtkBridgePointIterator::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x != nullptr);
  this->CurrentIterator->GetPosition(x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
svtkIdType svtkBridgePointIterator::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->CurrentIterator->GetId();
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over points of `ds'.
// \pre ds_exists: ds!=0
void svtkBridgePointIterator::InitWithDataSet(svtkBridgeDataSet* ds)
{
  assert("pre: ds_exists" && ds != nullptr);

  this->IteratorOnDataSet->InitWithDataSet(ds);
  this->CurrentIterator = this->IteratorOnDataSet;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over one point of identifier `id' on dataset `ds'.
// \pre ds_can_be_null: ds!=0 || ds==0
// \pre valid_id: svtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
void svtkBridgePointIterator::InitWithOnePoint(svtkBridgeDataSet* ds, svtkIdType id)
{
  assert("pre: valid_id" &&
    ((ds == nullptr) || ((id >= 0) && (id <= ds->GetNumberOfCells())))); // A=>B: !A||B

  this->IteratorOne->InitWithOnePoint(ds, id);
  this->CurrentIterator = this->IteratorOne;
}

//-----------------------------------------------------------------------------
// Description:
// The iterator will iterate over the point of a cell
// \pre cell_exists: cell!=0
void svtkBridgePointIterator::InitWithCell(svtkBridgeCell* cell)
{
  assert("pre: cell_exists" && cell != nullptr);

  this->IteratorOnCell->InitWithCell(cell);
  this->CurrentIterator = this->IteratorOnCell;
}
