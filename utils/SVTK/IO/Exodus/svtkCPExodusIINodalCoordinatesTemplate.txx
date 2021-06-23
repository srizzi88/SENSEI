/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIINodalCoordinatesTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCPExodusIINodalCoordinatesTemplate.h"

#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkVariant.h"
#include "svtkVariantCast.h"

//------------------------------------------------------------------------------
// Can't use svtkStandardNewMacro with a template.
template <class Scalar>
svtkCPExodusIINodalCoordinatesTemplate<Scalar>* svtkCPExodusIINodalCoordinatesTemplate<Scalar>::New()
{
  SVTK_STANDARD_NEW_BODY(svtkCPExodusIINodalCoordinatesTemplate<Scalar>);
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::PrintSelf(ostream& os, svtkIndent indent)
{
  this->svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Superclass::PrintSelf(os, indent);
  os << indent << "XArray: " << this->XArray << std::endl;
  os << indent << "YArray: " << this->YArray << std::endl;
  os << indent << "ZArray: " << this->ZArray << std::endl;
  os << indent << "TempDoubleArray: " << this->TempDoubleArray << std::endl;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Initialize()
{
  delete[] this->XArray;
  this->XArray = nullptr;
  delete[] this->YArray;
  this->YArray = nullptr;
  delete[] this->ZArray;
  this->ZArray = nullptr;
  delete[] this->TempDoubleArray;
  this->TempDoubleArray = nullptr;
  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetTuples(
  svtkIdList* ptIds, svtkAbstractArray* output)
{
  svtkDataArray* outArray = svtkDataArray::FastDownCast(output);
  if (!outArray)
  {
    svtkWarningMacro(<< "Input is not a svtkDataArray");
    return;
  }

  svtkIdType numTuples = ptIds->GetNumberOfIds();

  outArray->SetNumberOfComponents(this->NumberOfComponents);
  outArray->SetNumberOfTuples(numTuples);

  const svtkIdType numPoints = ptIds->GetNumberOfIds();
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    outArray->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetTuples(
  svtkIdType p1, svtkIdType p2, svtkAbstractArray* output)
{
  svtkDataArray* da = svtkDataArray::FastDownCast(output);
  if (!da)
  {
    svtkErrorMacro(<< "Input is not a svtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    svtkErrorMacro(<< "Incorrect number of components in input array.");
    return;
  }

  for (svtkIdType daTupleId = 0; p1 <= p2; ++p1)
  {
    da->SetTuple(daTupleId++, this->GetTuple(p1));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Squeeze()
{
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkArrayIterator* svtkCPExodusIINodalCoordinatesTemplate<Scalar>::NewIterator()
{
  svtkErrorMacro(<< "Not implemented.");
  return nullptr;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::LookupValue(svtkVariant value)
{
  bool valid = true;
  Scalar val = svtkVariantCast<Scalar>(value, &valid);
  if (valid)
  {
    return this->Lookup(val, 0);
  }
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::LookupValue(svtkVariant value, svtkIdList* ids)
{
  bool valid = true;
  Scalar val = svtkVariantCast<Scalar>(value, &valid);
  ids->Reset();
  if (valid)
  {
    svtkIdType index = 0;
    while ((index = this->Lookup(val, index)) >= 0)
    {
      ids->InsertNextId(index++);
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkVariant svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetVariantValue(svtkIdType idx)
{
  return svtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::ClearLookup()
{
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar>
double* svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetTuple(svtkIdType i)
{
  this->GetTuple(i, this->TempDoubleArray);
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetTuple(svtkIdType i, double* tuple)
{
  tuple[0] = static_cast<double>(this->XArray[i]);
  tuple[1] = static_cast<double>(this->YArray[i]);
  if (this->ZArray != nullptr)
  {
    tuple[2] = static_cast<double>(this->ZArray[i]);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::LookupTypedValue(Scalar value)
{
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::LookupTypedValue(Scalar value, svtkIdList* ids)
{
  ids->Reset();
  svtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0)
  {
    ids->InsertNextId(index++);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
typename svtkCPExodusIINodalCoordinatesTemplate<Scalar>::ValueType
svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetValue(svtkIdType idx) const
{
  return const_cast<svtkCPExodusIINodalCoordinatesTemplate<Scalar>*>(this)->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
typename svtkCPExodusIINodalCoordinatesTemplate<Scalar>::ValueType&
svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetValueReference(svtkIdType idx)
{
  const svtkIdType tuple = idx / this->NumberOfComponents;
  const svtkIdType comp = idx % this->NumberOfComponents;
  switch (comp)
  {
    case 0:
      return this->XArray[tuple];
    case 1:
      return this->YArray[tuple];
    case 2:
      return this->ZArray[tuple];
    default:
      svtkErrorMacro(<< "Invalid number of components.");
      static Scalar dummy(0);
      return dummy;
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetTypedTuple(
  svtkIdType tupleId, Scalar* tuple) const
{
  tuple[0] = this->XArray[tupleId];
  tuple[1] = this->YArray[tupleId];
  if (this->ZArray != nullptr)
  {
    tuple[2] = this->ZArray[tupleId];
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkTypeBool svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Allocate(svtkIdType, svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkTypeBool svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Resize(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetNumberOfTuples(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetTuple(
  svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetTuple(svtkIdType, const float*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetTuple(svtkIdType, const double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTuple(
  svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTuple(svtkIdType, const float*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTuple(svtkIdType, const double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTuples(
  svtkIdList*, svtkIdList*, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTuples(
  svtkIdType, svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertNextTuple(
  svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertNextTuple(const float*)
{

  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertNextTuple(const double*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::DeepCopy(svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::DeepCopy(svtkDataArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InterpolateTuple(
  svtkIdType, svtkIdList*, svtkAbstractArray*, double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InterpolateTuple(
  svtkIdType, svtkIdType, svtkAbstractArray*, svtkIdType, svtkAbstractArray*, double)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetVariantValue(svtkIdType, svtkVariant)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertVariantValue(svtkIdType, svtkVariant)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::RemoveTuple(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::RemoveFirstTuple()
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::RemoveLastTuple()
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetTypedTuple(svtkIdType, const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertTypedTuple(svtkIdType, const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertNextTypedTuple(const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetValue(svtkIdType, Scalar)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertNextValue(Scalar)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::InsertValue(svtkIdType, Scalar)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkCPExodusIINodalCoordinatesTemplate<Scalar>::svtkCPExodusIINodalCoordinatesTemplate()
  : XArray(nullptr)
  , YArray(nullptr)
  , ZArray(nullptr)
  , TempDoubleArray(nullptr)
{
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkCPExodusIINodalCoordinatesTemplate<Scalar>::~svtkCPExodusIINodalCoordinatesTemplate()
{
  delete[] this->XArray;
  delete[] this->YArray;
  delete[] this->ZArray;
  delete[] this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIINodalCoordinatesTemplate<Scalar>::SetExodusScalarArrays(
  Scalar* x, Scalar* y, Scalar* z, svtkIdType numPoints)
{
  Initialize();
  this->XArray = x;
  this->YArray = y;
  this->ZArray = z;
  this->NumberOfComponents = (z != nullptr) ? 3 : 2;
  this->Size = this->NumberOfComponents * numPoints;
  this->MaxId = this->Size - 1;
  this->TempDoubleArray = new double[this->NumberOfComponents];
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIINodalCoordinatesTemplate<Scalar>::Lookup(const Scalar& val, svtkIdType index)
{
  while (index <= this->MaxId)
  {
    if (this->GetValueReference(index++) == val)
    {
      return index;
    }
  }
  return -1;
}
