//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 Sandia Corporation.
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtkmDataArray_hxx
#define svtkmDataArray_hxx

#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayRangeCompute.h>

namespace internal
{
//=============================================================================
template <typename T>
class ArrayHandleWrapperBase
{
public:
  virtual ~ArrayHandleWrapperBase() = default;

  virtual svtkIdType GetNumberOfTuples() const = 0;
  virtual int GetNumberOfComponents() const = 0;

  virtual void SetTuple(svtkIdType idx, const T* value) = 0;
  virtual void GetTuple(svtkIdType idx, T* value) const = 0;

  virtual void SetComponent(svtkIdType tuple, int comp, const T& value) = 0;
  virtual T GetComponent(svtkIdType tuple, int comp) const = 0;

  virtual void Allocate(svtkIdType numTuples) = 0;
  virtual void Reallocate(svtkIdType numTuples) = 0;

  virtual svtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const = 0;
};

//-----------------------------------------------------------------------------
template <typename T,
  typename NumComponentsTags = typename svtkm::VecTraits<T>::HasMultipleComponents>
struct FlattenVec;

template <typename T>
struct FlattenVec<T, svtkm::VecTraitsTagMultipleComponents>
{
  using SubVec = FlattenVec<typename svtkm::VecTraits<T>::ComponentType>;
  using ComponentType = typename SubVec::ComponentType;

  static SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfComponents(const T& vec)
  {
    return svtkm::VecTraits<T>::GetNumberOfComponents(vec) *
      SubVec::GetNumberOfComponents(svtkm::VecTraits<T>::GetComponent(vec, 0));
  }

  static SVTKM_EXEC_CONT const ComponentType& GetComponent(const T& vec, svtkm::IdComponent comp)
  {
    auto ncomps = SubVec::GetNumberOfComponents(svtkm::VecTraits<T>::GetComponent(vec, 0));
    return SubVec::GetComponent(
      svtkm::VecTraits<T>::GetComponent(vec, comp / ncomps), comp % ncomps);
  }

  static SVTKM_EXEC_CONT ComponentType& GetComponent(T& vec, svtkm::IdComponent comp)
  {
    auto ncomps = SubVec::GetNumberOfComponents(svtkm::VecTraits<T>::GetComponent(vec, 0));
    return SubVec::GetComponent(
      svtkm::VecTraits<T>::GetComponent(vec, comp / ncomps), comp % ncomps);
  }
};

template <typename T>
struct FlattenVec<T, svtkm::VecTraitsTagSingleComponent>
{
  using ComponentType = typename svtkm::VecTraits<T>::ComponentType;

  static constexpr SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfComponents(const T&) { return 1; }

  static SVTKM_EXEC_CONT const ComponentType& GetComponent(const T& vec, svtkm::IdComponent)
  {
    return svtkm::VecTraits<T>::GetComponent(vec, 0);
  }

  static SVTKM_EXEC_CONT ComponentType& GetComponent(T& vec, svtkm::IdComponent)
  {
    return svtkm::VecTraits<T>::GetComponent(vec, 0);
  }
};

//-----------------------------------------------------------------------------
template <typename ValueType, typename StorageTag>
class ArrayHandleWrapper
  : public ArrayHandleWrapperBase<typename FlattenVec<ValueType>::ComponentType>
{
private:
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
  using ComponentType = typename FlattenVec<ValueType>::ComponentType;
  using PortalType = typename ArrayHandleType::PortalControl;

public:
  explicit ArrayHandleWrapper(const ArrayHandleType& handle)
    : Handle(handle)
  {
    this->Portal = this->Handle.GetPortalControl();
    this->NumberOfComponents = (this->Portal.GetNumberOfValues() == 0)
      ? 1
      : FlattenVec<ValueType>::GetNumberOfComponents(this->Portal.Get(0));
  }

  svtkIdType GetNumberOfTuples() const override { return this->Portal.GetNumberOfValues(); }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(svtkIdType idx, const ComponentType* value) override
  {
    // some vector types are not default constructible...
    auto v = this->Portal.Get(idx);
    for (svtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      FlattenVec<ValueType>::GetComponent(v, i) = value[i];
    }
    this->Portal.Set(idx, v);
  }

  void GetTuple(svtkIdType idx, ComponentType* value) const override
  {
    auto v = this->Portal.Get(idx);
    for (svtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      value[i] = FlattenVec<ValueType>::GetComponent(v, i);
    }
  }

  void SetComponent(svtkIdType tuple, int comp, const ComponentType& value) override
  {
    auto v = this->Portal.Get(tuple);
    FlattenVec<ValueType>::GetComponent(v, comp) = value;
    this->Portal.Set(tuple, v);
  }

  ComponentType GetComponent(svtkIdType tuple, int comp) const override
  {
    return FlattenVec<ValueType>::GetComponent(this->Portal.Get(tuple), comp);
  }

  void Allocate(svtkIdType numTuples) override
  {
    this->Handle.Allocate(numTuples);
    this->Portal = this->Handle.GetPortalControl();
  }

  void Reallocate(svtkIdType numTuples) override
  {
    ArrayHandleType newHandle;
    newHandle.Allocate(numTuples);
    svtkm::cont::Algorithm::CopySubRange(this->Handle, 0,
      std::min(this->Handle.GetNumberOfValues(), newHandle.GetNumberOfValues()), newHandle, 0);
    this->Handle = std::move(newHandle);
    this->Portal = this->Handle.GetPortalControl();
  }

  svtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return svtkm::cont::VariantArrayHandle{ this->Handle };
  }

private:
  ArrayHandleType Handle;
  PortalType Portal;
  svtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename ValueType, typename StorageTag>
class ArrayHandleWrapperReadOnly
  : public ArrayHandleWrapperBase<typename FlattenVec<ValueType>::ComponentType>
{
private:
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
  using ComponentType = typename FlattenVec<ValueType>::ComponentType;
  using PortalType = typename ArrayHandleType::PortalConstControl;

public:
  explicit ArrayHandleWrapperReadOnly(const ArrayHandleType& handle)
    : Handle(handle)
  {
    this->Portal = this->Handle.GetPortalConstControl();
    this->NumberOfComponents = (this->Portal.GetNumberOfValues() == 0)
      ? 1
      : FlattenVec<ValueType>::GetNumberOfComponents(this->Portal.Get(0));
  }

  svtkIdType GetNumberOfTuples() const override { return this->Portal.GetNumberOfValues(); }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(svtkIdType, const ComponentType*) override
  {
    svtkGenericWarningMacro(<< "SetTuple called on read-only svtkmDataArray");
  }

  void GetTuple(svtkIdType idx, ComponentType* value) const override
  {
    auto v = this->Portal.Get(idx);
    for (svtkm::IdComponent i = 0; i < this->NumberOfComponents; ++i)
    {
      value[i] = FlattenVec<ValueType>::GetComponent(v, i);
    }
  }

  void SetComponent(svtkIdType, int, const ComponentType&) override
  {
    svtkGenericWarningMacro(<< "SetComponent called on read-only svtkmDataArray");
  }

  ComponentType GetComponent(svtkIdType tuple, int comp) const override
  {
    return FlattenVec<ValueType>::GetComponent(this->Portal.Get(tuple), comp);
  }

  void Allocate(svtkIdType) override
  {
    svtkGenericWarningMacro(<< "Allocate called on read-only svtkmDataArray");
  }

  void Reallocate(svtkIdType) override
  {
    svtkGenericWarningMacro(<< "Reallocate called on read-only svtkmDataArray");
  }

  svtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return svtkm::cont::VariantArrayHandle{ this->Handle };
  }

private:
  ArrayHandleType Handle;
  PortalType Portal;
  svtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename T>
class ArrayHandleWrapperFlatSOA : public ArrayHandleWrapperBase<T>
{
private:
  using ArrayHandleType = svtkm::cont::ArrayHandle<T>;
  using PortalType = typename ArrayHandleType::PortalControl;
  using VtkmArrayType = svtkm::cont::ArrayHandleGroupVecVariable<ArrayHandleType,
    svtkm::cont::ArrayHandleCounting<svtkm::Id> >;

public:
  explicit ArrayHandleWrapperFlatSOA(const ArrayHandleType& handle, int numberOfComponents)
    : Handle(handle)
    , NumberOfComponents(numberOfComponents)
  {
    this->Portal = this->Handle.GetPortalControl();
  }

  svtkIdType GetNumberOfTuples() const override
  {
    return this->Portal.GetNumberOfValues() / this->NumberOfComponents;
  }

  int GetNumberOfComponents() const override { return this->NumberOfComponents; }

  void SetTuple(svtkIdType idx, const T* value) override
  {
    svtkm::Id start = idx * this->NumberOfComponents;
    svtkm::Id end = start + this->NumberOfComponents;
    for (auto i = start; i < end; ++i)
    {
      this->Portal.Set(i, *value++);
    }
  }

  void GetTuple(svtkIdType idx, T* value) const override
  {
    svtkm::Id start = idx * this->NumberOfComponents;
    svtkm::Id end = start + this->NumberOfComponents;
    for (auto i = start; i < end; ++i)
    {
      *value++ = this->Portal.Get(i);
    }
  }

  void SetComponent(svtkIdType tuple, int comp, const T& value) override
  {
    this->Portal.Set((tuple * this->NumberOfComponents) + comp, value);
  }

  T GetComponent(svtkIdType tuple, int comp) const override
  {
    return this->Portal.Get((tuple * this->NumberOfComponents) + comp);
  }

  void Allocate(svtkIdType numTuples) override
  {
    this->Handle.Allocate(numTuples * this->NumberOfComponents);
    this->Portal = this->Handle.GetPortalControl();
  }

  void Reallocate(svtkIdType numTuples) override
  {
    ArrayHandleType newHandle;
    newHandle.Allocate(numTuples * this->NumberOfComponents);
    svtkm::cont::Algorithm::CopySubRange(this->Handle, 0,
      std::min(this->Handle.GetNumberOfValues(), newHandle.GetNumberOfValues()), newHandle, 0);
    this->Handle = std::move(newHandle);
    this->Portal = this->Handle.GetPortalControl();
  }

  svtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const override
  {
    return svtkm::cont::VariantArrayHandle{ this->GetVtkmArray() };
  }

private:
  VtkmArrayType GetVtkmArray() const
  {
    auto length = this->Handle.GetNumberOfValues() / this->NumberOfComponents;
    auto offsets = svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, this->NumberOfComponents, length);
    return VtkmArrayType{ this->Handle, offsets };
  }

  ArrayHandleType Handle;
  PortalType Portal;
  svtkm::IdComponent NumberOfComponents;
};

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
using IsReadOnly = std::integral_constant<bool,
  !svtkm::cont::internal::IsWritableArrayHandle<ArrayHandleType>::value>;

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* WrapArrayHandle(
  const svtkm::cont::ArrayHandle<T, S>& ah, std::false_type)
{
  return new ArrayHandleWrapper<T, S>{ ah };
}

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* WrapArrayHandle(
  const svtkm::cont::ArrayHandle<T, S>& ah, std::true_type)
{
  return new ArrayHandleWrapperReadOnly<T, S>{ ah };
}

template <typename T, typename S>
ArrayHandleWrapperBase<typename FlattenVec<T>::ComponentType>* MakeArrayHandleWrapper(
  const svtkm::cont::ArrayHandle<T, S>& ah)
{
  return WrapArrayHandle(ah, typename IsReadOnly<svtkm::cont::ArrayHandle<T, S> >::type{});
}

template <typename T>
ArrayHandleWrapperBase<T>* MakeArrayHandleWrapper(svtkIdType numberOfTuples, int numberOfComponents)
{
  switch (numberOfComponents)
  {
    case 1:
    {
      svtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 2:
    {
      svtkm::cont::ArrayHandle<svtkm::Vec<T, 2> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 3:
    {
      svtkm::cont::ArrayHandle<svtkm::Vec<T, 3> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    case 4:
    {
      svtkm::cont::ArrayHandle<svtkm::Vec<T, 4> > ah;
      ah.Allocate(numberOfTuples);
      return MakeArrayHandleWrapper(ah);
    }
    default:
    {
      svtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples * numberOfComponents);
      return new ArrayHandleWrapperFlatSOA<T>{ ah, numberOfComponents };
    }
  }
}

} // internal

//=============================================================================
template <typename T>
svtkmDataArray<T>::svtkmDataArray() = default;

template <typename T>
svtkmDataArray<T>::~svtkmDataArray() = default;

template <typename T>
svtkmDataArray<T>* svtkmDataArray<T>::New()
{
  SVTK_STANDARD_NEW_BODY(svtkmDataArray<T>);
}

template <typename T>
template <typename V, typename S>
void svtkmDataArray<T>::SetVtkmArrayHandle(const svtkm::cont::ArrayHandle<V, S>& ah)
{
  static_assert(std::is_same<T, typename internal::FlattenVec<V>::ComponentType>::value,
    "Component type of the arrays don't match");

  this->VtkmArray.reset(internal::MakeArrayHandleWrapper(ah));

  this->Size = this->VtkmArray->GetNumberOfTuples() * this->VtkmArray->GetNumberOfComponents();
  this->MaxId = this->Size - 1;
  this->SetNumberOfComponents(this->VtkmArray->GetNumberOfComponents());
}

template <typename T>
svtkm::cont::VariantArrayHandle svtkmDataArray<T>::GetVtkmVariantArrayHandle() const
{
  return this->VtkmArray->GetVtkmVariantArrayHandle();
}

template <typename T>
auto svtkmDataArray<T>::GetValue(svtkIdType valueIdx) const -> ValueType
{
  auto idx = valueIdx / this->NumberOfComponents;
  auto comp = valueIdx % this->NumberOfComponents;
  return this->VtkmArray->GetComponent(idx, comp);
}

template <typename T>
void svtkmDataArray<T>::SetValue(svtkIdType valueIdx, ValueType value)
{
  auto idx = valueIdx / this->NumberOfComponents;
  auto comp = valueIdx % this->NumberOfComponents;
  this->VtkmArray->SetComponent(idx, comp, value);
}

template <typename T>
void svtkmDataArray<T>::GetTypedTuple(svtkIdType tupleIdx, ValueType* tuple) const
{
  this->VtkmArray->GetTuple(tupleIdx, tuple);
}

template <typename T>
void svtkmDataArray<T>::SetTypedTuple(svtkIdType tupleIdx, const ValueType* tuple)
{
  this->VtkmArray->SetTuple(tupleIdx, tuple);
}

template <typename T>
auto svtkmDataArray<T>::GetTypedComponent(svtkIdType tupleIdx, int compIdx) const -> ValueType
{
  return this->VtkmArray->GetComponent(tupleIdx, compIdx);
}

template <typename T>
void svtkmDataArray<T>::SetTypedComponent(svtkIdType tupleIdx, int compIdx, ValueType value)
{
  this->VtkmArray->SetComponent(tupleIdx, compIdx, value);
}

template <typename T>
bool svtkmDataArray<T>::AllocateTuples(svtkIdType numTuples)
{
  if (this->VtkmArray && this->VtkmArray->GetNumberOfComponents() == this->NumberOfComponents)
  {
    this->VtkmArray->Allocate(numTuples);
  }
  else
  {
    this->VtkmArray.reset(internal::MakeArrayHandleWrapper<T>(numTuples, this->NumberOfComponents));
  }
  return true;
}

template <typename T>
bool svtkmDataArray<T>::ReallocateTuples(svtkIdType numTuples)
{
  this->VtkmArray->Reallocate(numTuples);
  return true;
}

#endif // svtkmDataArray_hxx
