/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIResultsArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCPExodusIIResultsArrayTemplate.h"

#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkVariant.h"
#include "svtkVariantCast.h"

//------------------------------------------------------------------------------
// Can't use svtkStandardNewMacro on a templated class.
template <class Scalar>
svtkCPExodusIIResultsArrayTemplate<Scalar>* svtkCPExodusIIResultsArrayTemplate<Scalar>::New()
{
  SVTK_STANDARD_NEW_BODY(svtkCPExodusIIResultsArrayTemplate<Scalar>);
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::PrintSelf(ostream& os, svtkIndent indent)
{
  this->svtkCPExodusIIResultsArrayTemplate<Scalar>::Superclass::PrintSelf(os, indent);

  os << indent << "Number of arrays: " << this->Arrays.size() << "\n";
  svtkIndent deeper = indent.GetNextIndent();
  for (size_t i = 0; i < this->Arrays.size(); ++i)
  {
    os << deeper << "Array " << i << ": " << this->Arrays.at(i) << "\n";
  }

  os << indent << "TempDoubleArray: " << this->TempDoubleArray << "\n";
  os << indent << "Save: " << this->Save << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetExodusScalarArrays(
  std::vector<Scalar*> arrays, svtkIdType numTuples)
{
  this->Initialize();
  this->NumberOfComponents = static_cast<int>(arrays.size());
  this->Arrays = arrays;
  this->Size = this->NumberOfComponents * numTuples;
  this->MaxId = this->Size - 1;
  this->TempDoubleArray = new double[this->NumberOfComponents];
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetExodusScalarArrays(
  std::vector<Scalar*> arrays, svtkIdType numTuples, bool save)
{
  this->SetExodusScalarArrays(arrays, numTuples);
  this->Save = save;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::Initialize()
{
  if (!this->Save)
  {
    for (size_t i = 0; i < this->Arrays.size(); ++i)
    {
      delete this->Arrays[i];
    }
  }
  this->Arrays.clear();
  this->Arrays.push_back(nullptr);

  delete[] this->TempDoubleArray;
  this->TempDoubleArray = nullptr;

  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 1;
  // the default is to have this class delete the arrays when done with them.
  this->Save = false;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::GetTuples(
  svtkIdList* ptIds, svtkAbstractArray* output)
{
  svtkDataArray* da = svtkDataArray::FastDownCast(output);
  if (!da)
  {
    svtkWarningMacro(<< "Input is not a svtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    svtkWarningMacro(<< "Incorrect number of components in input array.");
    return;
  }

  const svtkIdType numPoints = ptIds->GetNumberOfIds();
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    da->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::GetTuples(
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
void svtkCPExodusIIResultsArrayTemplate<Scalar>::Squeeze()
{
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkArrayIterator* svtkCPExodusIIResultsArrayTemplate<Scalar>::NewIterator()
{
  svtkErrorMacro(<< "Not implemented.");
  return nullptr;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::LookupValue(svtkVariant value)
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
void svtkCPExodusIIResultsArrayTemplate<Scalar>::LookupValue(svtkVariant value, svtkIdList* ids)
{
  bool valid = true;
  Scalar val = svtkVariantCast<Scalar>(value, &valid);
  ids->Reset();
  if (valid)
  {
    svtkIdType index = 0;
    while ((index = this->Lookup(val, index)) >= 0)
    {
      ids->InsertNextId(index);
      ++index;
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkVariant svtkCPExodusIIResultsArrayTemplate<Scalar>::GetVariantValue(svtkIdType idx)
{
  return svtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::ClearLookup()
{
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar>
double* svtkCPExodusIIResultsArrayTemplate<Scalar>::GetTuple(svtkIdType i)
{
  this->GetTuple(i, this->TempDoubleArray);
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::GetTuple(svtkIdType i, double* tuple)
{
  for (size_t comp = 0; comp < this->Arrays.size(); ++comp)
  {
    tuple[comp] = static_cast<double>(this->Arrays[comp][i]);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::LookupTypedValue(Scalar value)
{
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::LookupTypedValue(Scalar value, svtkIdList* ids)
{
  ids->Reset();
  svtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0)
  {
    ids->InsertNextId(index);
    ++index;
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
typename svtkCPExodusIIResultsArrayTemplate<Scalar>::ValueType
svtkCPExodusIIResultsArrayTemplate<Scalar>::GetValue(svtkIdType idx) const
{
  return const_cast<svtkCPExodusIIResultsArrayTemplate<Scalar>*>(this)->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
typename svtkCPExodusIIResultsArrayTemplate<Scalar>::ValueType&
svtkCPExodusIIResultsArrayTemplate<Scalar>::GetValueReference(svtkIdType idx)
{
  const svtkIdType tuple = idx / this->NumberOfComponents;
  const svtkIdType comp = idx % this->NumberOfComponents;
  return this->Arrays[comp][tuple];
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::GetTypedTuple(
  svtkIdType tupleId, Scalar* tuple) const
{
  for (size_t comp = 0; comp < this->Arrays.size(); ++comp)
  {
    tuple[comp] = this->Arrays[comp][tupleId];
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkTypeBool svtkCPExodusIIResultsArrayTemplate<Scalar>::Allocate(svtkIdType, svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkTypeBool svtkCPExodusIIResultsArrayTemplate<Scalar>::Resize(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetNumberOfTuples(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetTuple(svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetTuple(svtkIdType, const float*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetTuple(svtkIdType, const double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTuple(svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTuple(svtkIdType, const float*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTuple(svtkIdType, const double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTuples(
  svtkIdList*, svtkIdList*, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTuples(
  svtkIdType, svtkIdType, svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertNextTuple(svtkIdType, svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertNextTuple(const float*)
{

  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertNextTuple(const double*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::DeepCopy(svtkAbstractArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::DeepCopy(svtkDataArray*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InterpolateTuple(
  svtkIdType, svtkIdList*, svtkAbstractArray*, double*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InterpolateTuple(
  svtkIdType, svtkIdType, svtkAbstractArray*, svtkIdType, svtkAbstractArray*, double)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetVariantValue(svtkIdType, svtkVariant)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertVariantValue(svtkIdType, svtkVariant)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::RemoveTuple(svtkIdType)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::RemoveFirstTuple()
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::RemoveLastTuple()
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetTypedTuple(svtkIdType, const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertTypedTuple(svtkIdType, const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertNextTypedTuple(const Scalar*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::SetValue(svtkIdType, Scalar)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertNextValue(Scalar)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void svtkCPExodusIIResultsArrayTemplate<Scalar>::InsertValue(svtkIdType, Scalar)
{
  svtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkCPExodusIIResultsArrayTemplate<Scalar>::svtkCPExodusIIResultsArrayTemplate()
  : TempDoubleArray(nullptr)
  , Save(false)
{
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkCPExodusIIResultsArrayTemplate<Scalar>::~svtkCPExodusIIResultsArrayTemplate()
{
  typedef typename std::vector<Scalar*>::const_iterator ArrayIterator;
  if (!this->Save)
  {
    for (ArrayIterator it = this->Arrays.begin(), itEnd = this->Arrays.end(); it != itEnd; ++it)
    {
      delete[] * it;
    }
  }
  delete[] this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
svtkIdType svtkCPExodusIIResultsArrayTemplate<Scalar>::Lookup(const Scalar& val, svtkIdType index)
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
