/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgeAttribute - Implementation of svtkGenericAttribute.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericAttribute, svtkBridgeDataSet

#include "svtkBridgeAttribute.h"

#include "svtkBridgeCell.h"
#include "svtkBridgeCellIterator.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkGenericCell.h"
#include "svtkGenericPointIterator.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSetGet.h"

#include <cassert>

svtkStandardNewMacro(svtkBridgeAttribute);

void svtkBridgeAttribute::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Name of the attribute. (e.g. "velocity")
// \post result_may_not_exist: result!=0 || result==0
const char* svtkBridgeAttribute::GetName()
{
  return this->Data->GetArray(this->AttributeNumber)->GetName();
}

//-----------------------------------------------------------------------------
// Description:
// Dimension of the attribute. (1 for scalar, 3 for velocity)
// \post positive_result: result>=0
int svtkBridgeAttribute::GetNumberOfComponents()
{
  int result = this->Data->GetArray(this->AttributeNumber)->GetNumberOfComponents();
  assert("post: positive_result" && result >= 0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Is the attribute centered either on points, cells or boundaries?
// \post valid_result: (result==svtkPointCentered) ||
//            (result==svtkCellCentered) || (result==svtkBoundaryCentered)
int svtkBridgeAttribute::GetCentering()
{
  int result;
  if (this->Pd != nullptr)
  {
    result = svtkPointCentered;
  }
  else
  {
    result = svtkCellCentered;
  }
  assert("post: valid_result" &&
    ((result == svtkPointCentered) || (result == svtkCellCentered) ||
      (result == svtkBoundaryCentered)));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Type of the attribute: scalar, vector, normal, texture coordinate, tensor
// \post valid_result: (result==svtkDataSetAttributes::SCALARS)
//                   ||(result==svtkDataSetAttributes::VECTORS)
//                   ||(result==svtkDataSetAttributes::NORMALS)
//                   ||(result==svtkDataSetAttributes::TCOORDS)
//                   ||(result==svtkDataSetAttributes::TENSORS)
int svtkBridgeAttribute::GetType()
{
  int result = this->Data->IsArrayAnAttribute(this->AttributeNumber);
  if (result == -1)
  {
    switch (this->GetNumberOfComponents())
    {
      case 1:
        result = svtkDataSetAttributes::SCALARS;
        break;
      case 3:
        result = svtkDataSetAttributes::VECTORS;
        break;
      case 9:
        result = svtkDataSetAttributes::TENSORS;
        break;
      default:
        assert("check: unknown attribute type" && 0);
        break;
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Type of the components of the attribute: int, float, double
// \post valid_result: (result==SVTK_BIT)           ||(result==SVTK_CHAR)
//                   ||(result==SVTK_UNSIGNED_CHAR) ||(result==SVTK_SHORT)
//                   ||(result==SVTK_UNSIGNED_SHORT)||(result==SVTK_INT)
//                   ||(result==SVTK_UNSIGNED_INT)  ||(result==SVTK_LONG)
//                   ||(result==SVTK_UNSIGNED_LONG) ||(result==SVTK_FLOAT)
//                   ||(result==SVTK_DOUBLE)        ||(result==SVTK_ID_TYPE)
int svtkBridgeAttribute::GetComponentType()
{
  return this->Data->GetArray(this->AttributeNumber)->GetDataType();
}

//-----------------------------------------------------------------------------
// Description:
// Number of tuples.
// \post valid_result: result>=0
svtkIdType svtkBridgeAttribute::GetSize()
{
  svtkIdType result = this->Data->GetArray(this->AttributeNumber)->GetNumberOfTuples();
  assert("post: valid_result" && result >= 0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Size in kibibytes (1024 bytes) taken by the attribute.
unsigned long svtkBridgeAttribute::GetActualMemorySize()
{
  return this->Data->GetArray(this->AttributeNumber)->GetActualMemorySize();
}

//-----------------------------------------------------------------------------
// Description:
// Range of the attribute component `component'. It returns double, even if
// GetType()==SVTK_INT.
// NOT THREAD SAFE
// \pre valid_component: (component>=-1)&&(component<GetNumberOfComponents())
// \post result_exists: result!=0
double* svtkBridgeAttribute::GetRange(int component)
{
  assert(
    "pre: valid_component" && (component >= -1) && (component < this->GetNumberOfComponents()));
  double* result = this->Data->GetArray(this->AttributeNumber)->GetRange(component);
  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Range of the attribute component `component'.
// THREAD SAFE
// \pre valid_component: (component>=-1)&&(component<GetNumberOfComponents())
void svtkBridgeAttribute::GetRange(int component, double range[2])
{
  assert(
    "pre: valid_component" && (component >= -1) && (component < this->GetNumberOfComponents()));
  this->Data->GetArray(this->AttributeNumber)->GetRange(range, component);
}

//-----------------------------------------------------------------------------
// Description:
// Return the maximum euclidean norm for the tuples.
// \post positive_result: result>=0
double svtkBridgeAttribute::GetMaxNorm()
{
  double result = this->Data->GetArray(this->AttributeNumber)->GetMaxNorm();
  assert("post: positive_result" && result >= 0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Attribute at all points of cell `c'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
double* svtkBridgeAttribute::GetTuple(svtkGenericAdaptorCell* c)
{
  assert("pre: c_exists" && c != nullptr);

  this->AllocateInternalTuple(c->GetNumberOfPoints() * this->GetNumberOfComponents());
  this->GetTuple(c, this->InternalTuple);

  assert("post: result_exists" && this->InternalTuple != nullptr);
  return this->InternalTuple;
}

//-----------------------------------------------------------------------------
// Description:
// Put attribute at all points of cell `c' in `tuple'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
void svtkBridgeAttribute::GetTuple(svtkGenericAdaptorCell* c, double* tuple)
{
  assert("pre: c_exists" && c != nullptr);
  assert("pre: tuple_exists" && tuple != nullptr);

  double* p = tuple;
  int i;
  int j;
  int size;
  svtkBridgeCell* c2 = static_cast<svtkBridgeCell*>(c);

  if (this->Pd != nullptr)
  {
    i = 0;
    size = c2->GetNumberOfPoints();
    while (i < size)
    {
      j = c2->Cell->GetPointId(i);
      this->Data->GetArray(this->AttributeNumber)->GetTuple(j, p);
      ++i;
      p = p + this->GetNumberOfComponents();
    }
  }
  else
  {
    this->Data->GetArray(this->AttributeNumber)->GetTuple(c2->GetId(), tuple);
    // duplicate:
    size = c2->GetNumberOfPoints();
    i = 1;
    p = p + this->GetNumberOfComponents();
    while (i < size)
    {
      memcpy(p, tuple, sizeof(double) * this->GetNumberOfComponents());
      p = p + this->GetNumberOfComponents();
      ++i;
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
// Attribute at all points of cell `c'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
double* svtkBridgeAttribute::GetTuple(svtkGenericCellIterator* c)
{
  assert("pre: c_exists" && c != nullptr);
  assert("pre: c_valid" && !c->IsAtEnd());

  return this->GetTuple(c->GetCell());
}

//-----------------------------------------------------------------------------
// Description:
// Put attribute at all points of cell `c' in `tuple'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
void svtkBridgeAttribute::GetTuple(svtkGenericCellIterator* c, double* tuple)
{
  assert("pre: c_exists" && c != nullptr);
  assert("pre: c_valid" && !c->IsAtEnd());
  assert("pre: tuple_exists" && tuple != nullptr);

  this->GetTuple(c->GetCell(), tuple);
}

//-----------------------------------------------------------------------------
// Description:
// Value of the attribute at position `p'.
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result_size: sizeof(result)==GetNumberOfComponents()
double* svtkBridgeAttribute::GetTuple(svtkGenericPointIterator* p)
{
  assert("pre: p_exists" && p != nullptr);
  assert("pre: p_valid" && !p->IsAtEnd());

  this->AllocateInternalTuple(this->GetNumberOfComponents());

  this->Data->GetArray(this->AttributeNumber)->GetTuple(p->GetId(), this->InternalTuple);

  assert("post: result_exists" && this->InternalTuple != nullptr);
  return this->InternalTuple;
}

//-----------------------------------------------------------------------------
// Description:
// Put the value of the attribute at position `p' into `tuple'.
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple_size: sizeof(tuple)>=GetNumberOfComponents()
void svtkBridgeAttribute::GetTuple(svtkGenericPointIterator* p, double* tuple)
{
  assert("pre: p_exists" && p != nullptr);
  assert("pre: p_valid" && !p->IsAtEnd());
  assert("pre: tuple_exists" && tuple != nullptr);
  this->Data->GetArray(this->AttributeNumber)->GetTuple(p->GetId(), tuple);
}

//-----------------------------------------------------------------------------
// Description:
// Put component `i' of the attribute at all points of cell `c' in `values'.
// \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre values_exist: values!=0
// \pre valid_values: sizeof(values)>=c->GetCell()->GetNumberOfPoints()
void svtkBridgeAttribute::GetComponent(int i, svtkGenericCellIterator* c, double* values)
{
  assert("pre: c_exists" && c != nullptr);
  assert("pre: c_valid" && !c->IsAtEnd());

  int j;
  int size;
  svtkBridgeCellIterator* c2 = static_cast<svtkBridgeCellIterator*>(c);

  if (this->Pd != nullptr)
  {
    j = 0;
    size = c2->GetCell()->GetNumberOfPoints();
    while (j < size)
    {
      svtkIdType id = static_cast<svtkBridgeCell*>(c2->GetCell())->Cell->GetPointId(j);
      values[j] = this->Data->GetArray(this->AttributeNumber)->GetComponent(id, i);
      ++j;
    }
  }
  else
  {
    values[0] =
      this->Data->GetArray(this->AttributeNumber)->GetComponent(c2->GetCell()->GetId(), i);
    // duplicate:
    size = c2->GetCell()->GetNumberOfPoints();
    j = 1;
    while (j < size)
    {
      values[j] = values[0];
      ++j;
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
// Value of the component `i' of the attribute at position `p'.
// \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
double svtkBridgeAttribute::GetComponent(int i, svtkGenericPointIterator* p)
{
  assert("pre: p_exists" && p != nullptr);
  assert("pre: p_valid" && !p->IsAtEnd());
  // Only relevant if GetCentering()==svtkCenteringPoint?
  return this->Data->GetArray(this->AttributeNumber)->GetComponent(p->GetId(), i);
}

//-----------------------------------------------------------------------------
// Description:
// Recursive duplication of `other' in `this'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void svtkBridgeAttribute::DeepCopy(svtkGenericAttribute* other)
{
  assert("pre: other_exists" && other != nullptr);
  assert("pre: not_self" && other != this);
  svtkBridgeAttribute* o = static_cast<svtkBridgeAttribute*>(other);

  svtkSetObjectBodyMacro(Pd, svtkPointData, o->Pd);
  svtkSetObjectBodyMacro(Cd, svtkCellData, o->Cd);
  this->Data = o->Data;
  this->AttributeNumber = o->AttributeNumber;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Update `this' using fields of `other'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void svtkBridgeAttribute::ShallowCopy(svtkGenericAttribute* other)
{
  assert("pre: other_exists" && other != nullptr);
  assert("pre: not_self" && other != this);
  svtkBridgeAttribute* o = static_cast<svtkBridgeAttribute*>(other);

  svtkSetObjectBodyMacro(Pd, svtkPointData, o->Pd);
  svtkSetObjectBodyMacro(Cd, svtkCellData, o->Cd);
  this->Data = o->Data;
  this->AttributeNumber = o->AttributeNumber;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Set the current attribute to be centered on points with attribute `i' of
// `d'.
// \pre d_exists: d!=0
// \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
void svtkBridgeAttribute::InitWithPointData(svtkPointData* d, int i)
{
  assert("pre: d_exists" && d != nullptr);
  assert("pre: valid_range" && (i >= 0) && (i < d->GetNumberOfArrays()));
  svtkSetObjectBodyMacro(Cd, svtkCellData, 0);
  svtkSetObjectBodyMacro(Pd, svtkPointData, d);
  this->Data = d;
  this->AttributeNumber = i;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Set the current attribute to be centered on cells with attribute `i' of `d'.
// \pre d_exists: d!=0
// \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
void svtkBridgeAttribute::InitWithCellData(svtkCellData* d, int i)
{
  assert("pre: d_exists" && d != nullptr);
  assert("pre: valid_range" && (i >= 0) && (i < d->GetNumberOfArrays()));
  svtkSetObjectBodyMacro(Pd, svtkPointData, 0);
  svtkSetObjectBodyMacro(Cd, svtkCellData, d);
  this->Data = d;
  this->AttributeNumber = i;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Default constructor: empty attribute, not valid
svtkBridgeAttribute::svtkBridgeAttribute()
{
  this->Pd = nullptr;
  this->Cd = nullptr;
  this->Data = nullptr;
  this->AttributeNumber = 0;
  this->InternalTuple = nullptr;
  this->InternalTupleCapacity = 0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
svtkBridgeAttribute::~svtkBridgeAttribute()
{
  if (this->Pd != nullptr)
  {
    this->Pd->Delete();
  }
  else
  {
    if (this->Cd != nullptr)
    {
      this->Cd->Delete();
    }
  }
  delete[] this->InternalTuple;
}

//-----------------------------------------------------------------------------
// Description:
// If size>InternalTupleCapacity, allocate enough memory.
// \pre positive_size: size>0
void svtkBridgeAttribute::AllocateInternalTuple(int size)
{
  // size=this->GetNumberOfComponents()
  assert("pre: positive_size" && size > 0);

  if (this->InternalTuple == nullptr)
  {
    this->InternalTupleCapacity = size;
    this->InternalTuple = new double[this->InternalTupleCapacity];
  }
  else
  {
    if (InternalTupleCapacity < size)
    {
      this->InternalTupleCapacity = size;
      delete[] this->InternalTuple;
      this->InternalTuple = new double[this->InternalTupleCapacity];
    }
  }
}
