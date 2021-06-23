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
#ifndef __SVTK_WRAP__
#ifndef SVTK_WRAPPING_CXX

#ifndef svtkmDataArray_h
#define svtkmDataArray_h

#include "svtkAcceleratorsSVTKmModule.h" // For export macro
#include "svtkGenericDataArray.h"
#include "svtkmConfig.h" // For template export

#include <svtkm/List.h>                    // For svtkm::List
#include <svtkm/VecFromPortal.h>           // For svtkm::VecFromPortal
#include <svtkm/VecTraits.h>               // For svtkm::VecTraits
#include <svtkm/cont/ArrayHandle.h>        // For svtkm::cont::ArrayHandle
#include <svtkm/cont/VariantArrayHandle.h> // For svtkm::cont::VariantArrayHandle

#include <memory> // For unique_ptr

namespace internal
{

template <typename T>
class ArrayHandleWrapperBase;

} // internal

template <typename T>
class svtkmDataArray : public svtkGenericDataArray<svtkmDataArray<T>, T>
{
  static_assert(std::is_arithmetic<T>::value, "T must be an integral or floating-point type");

  using GenericDataArrayType = svtkGenericDataArray<svtkmDataArray<T>, T>;

public:
  using SelfType = svtkmDataArray<T>;
  svtkTemplateTypeMacro(SelfType, GenericDataArrayType);

  using typename Superclass::ValueType;

  using VtkmTypesList = svtkm::List<T, svtkm::Vec<T, 2>, svtkm::Vec<T, 3>, svtkm::Vec<T, 4>,
    svtkm::VecFromPortal<typename svtkm::cont::ArrayHandle<T>::PortalControl> >;

  static svtkmDataArray* New();

  template <typename V, typename S>
  void SetVtkmArrayHandle(const svtkm::cont::ArrayHandle<V, S>& ah);

  svtkm::cont::VariantArrayHandle GetVtkmVariantArrayHandle() const;

  /// concept methods for \c svtkGenericDataArray
  ValueType GetValue(svtkIdType valueIdx) const;
  void SetValue(svtkIdType valueIdx, ValueType value);
  void GetTypedTuple(svtkIdType tupleIdx, ValueType* tuple) const;
  void SetTypedTuple(svtkIdType tupleIdx, const ValueType* tuple);
  ValueType GetTypedComponent(svtkIdType tupleIdx, int compIdx) const;
  void SetTypedComponent(svtkIdType tupleIdx, int compIdx, ValueType value);

protected:
  svtkmDataArray();
  ~svtkmDataArray() override;

  /// concept methods for \c svtkGenericDataArray
  bool AllocateTuples(svtkIdType numTuples);
  bool ReallocateTuples(svtkIdType numTuples);

private:
  svtkmDataArray(const svtkmDataArray&) = delete;
  void operator=(const svtkmDataArray&) = delete;

  // To access AllocateTuples and ReallocateTuples
  friend Superclass;

  std::unique_ptr<internal::ArrayHandleWrapperBase<T> > VtkmArray;
};

//=============================================================================
template <typename T, typename S>
inline svtkmDataArray<typename svtkm::VecTraits<T>::BaseComponentType>* make_svtkmDataArray(
  const svtkm::cont::ArrayHandle<T, S>& ah)
{
  auto ret = svtkmDataArray<typename svtkm::VecTraits<T>::BaseComponentType>::New();
  ret->SetVtkmArrayHandle(ah);
  return ret;
}

//=============================================================================
#ifndef svtkmDataArray_cxx
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<char>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<double>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<float>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<int>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<long>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<long long>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<short>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<signed char>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<unsigned char>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<unsigned int>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<unsigned long>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<unsigned long long>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT svtkmDataArray<unsigned short>;
#endif // svtkmDataArray_cxx

#endif // svtkmDataArray_h

#include "svtkmDataArray.hxx"

#endif
#endif
// SVTK-HeaderTest-Exclude: svtkmDataArray.h
