/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOnCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgePointIteratorOnCell - Implementation of svtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericPointIterator, svtkBridgeDataSet

#include "svtkBridgePointIteratorOnCell.h"

#include <cassert>

#include "svtkBridgeCell.h"
#include "svtkBridgeDataSet.h"
#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkBridgePointIteratorOnCell);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
svtkBridgePointIteratorOnCell::svtkBridgePointIteratorOnCell()
{
  this->DataSet = nullptr;
  this->Cursor = 0;
  this->PtIds = nullptr;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
svtkBridgePointIteratorOnCell::~svtkBridgePointIteratorOnCell()
{
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, 0);
}

//-----------------------------------------------------------------------------
void svtkBridgePointIteratorOnCell::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgePointIteratorOnCell::Begin()
{
  if (this->PtIds != nullptr)
  {
    this->Cursor = 0;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
svtkTypeBool svtkBridgePointIteratorOnCell::IsAtEnd()
{
  return (this->PtIds == nullptr) || (this->Cursor >= this->PtIds->GetNumberOfIds());
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void svtkBridgePointIteratorOnCell::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Cursor++;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double* svtkBridgePointIteratorOnCell::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());

  double* result = this->DataSet->Implementation->GetPoint(this->PtIds->GetId(this->Cursor));

  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void svtkBridgePointIteratorOnCell::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x != nullptr);
  this->DataSet->Implementation->GetPoint(this->PtIds->GetId(this->Cursor), x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
svtkIdType svtkBridgePointIteratorOnCell::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->PtIds->GetId(this->Cursor);
}

//-----------------------------------------------------------------------------
// Description:
// The iterator will iterate over the point of a cell
// \pre cell_exists: cell!=0
void svtkBridgePointIteratorOnCell::InitWithCell(svtkBridgeCell* cell)
{
  assert("pre: cell_exists" && cell != nullptr);

  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, cell->DataSet);
  this->PtIds = cell->Cell->GetPointIds();
}
