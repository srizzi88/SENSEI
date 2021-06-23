/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOnDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgePointIteratorOnDataSet - Implementation of svtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericPointIterator, svtkBridgeDataSet

#include "svtkBridgePointIteratorOnDataSet.h"

#include <cassert>

#include "svtkBridgeDataSet.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkBridgePointIteratorOnDataSet);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
svtkBridgePointIteratorOnDataSet::svtkBridgePointIteratorOnDataSet()
{
  this->DataSet = nullptr;
  this->Size = 0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
svtkBridgePointIteratorOnDataSet::~svtkBridgePointIteratorOnDataSet()
{
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, 0);
}

//-----------------------------------------------------------------------------
void svtkBridgePointIteratorOnDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgePointIteratorOnDataSet::Begin()
{
  this->Id = 0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
svtkTypeBool svtkBridgePointIteratorOnDataSet::IsAtEnd()
{
  return (this->Id < 0) || (this->Id >= this->Size);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void svtkBridgePointIteratorOnDataSet::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Id++;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double* svtkBridgePointIteratorOnDataSet::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());

  double* result = this->DataSet->Implementation->GetPoint(this->Id);

  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void svtkBridgePointIteratorOnDataSet::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x != nullptr);
  this->DataSet->Implementation->GetPoint(this->Id, x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
svtkIdType svtkBridgePointIteratorOnDataSet::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->Id;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over points of `ds'.
// \pre ds_exists: ds!=0
void svtkBridgePointIteratorOnDataSet::InitWithDataSet(svtkBridgeDataSet* ds)
{
  assert("pre: ds_exists" && ds != nullptr);

  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, ds);
  this->Size = ds->GetNumberOfPoints();
}
