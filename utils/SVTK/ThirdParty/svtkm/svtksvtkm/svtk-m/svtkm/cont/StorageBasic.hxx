//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageBasic_hxx
#define svtk_m_cont_StorageBasic_hxx

#include <svtkm/cont/StorageBasic.h>

#include <limits>

namespace svtkm
{
namespace cont
{
namespace internal
{


template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>::Storage()
  : StorageBasicBase()
{
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>::Storage(const Storage<T, svtkm::cont::StorageTagBasic>& src)
  : StorageBasicBase(src)
{
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>::Storage(
  Storage<T, svtkm::cont::StorageTagBasic>&& src) noexcept : StorageBasicBase(std::move(src))
{
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>::Storage(const T* array, svtkm::Id numberOfValues)
  : StorageBasicBase(const_cast<T*>(array), numberOfValues, sizeof(T))
{
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>::Storage(const T* array,
                                                 svtkm::Id numberOfValues,
                                                 void (*deleteFunction)(void*))
  : StorageBasicBase(const_cast<T*>(array), numberOfValues, sizeof(T), deleteFunction)
{
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>& Storage<T, svtkm::cont::StorageTagBasic>::Storage::
operator=(const Storage<T, svtkm::cont::StorageTagBasic>& src)
{
  return static_cast<Storage<T, svtkm::cont::StorageTagBasic>&>(StorageBasicBase::operator=(src));
}

template <typename T>
Storage<T, svtkm::cont::StorageTagBasic>& Storage<T, svtkm::cont::StorageTagBasic>::Storage::
operator=(Storage<T, svtkm::cont::StorageTagBasic>&& src)
{
  return static_cast<Storage<T, svtkm::cont::StorageTagBasic>&>(
    StorageBasicBase::operator=(std::move(src)));
}


template <typename T>
void Storage<T, svtkm::cont::StorageTagBasic>::Allocate(svtkm::Id numberOfValues)
{
  this->AllocateValues(numberOfValues, sizeof(T));
}

template <typename T>
typename Storage<T, svtkm::cont::StorageTagBasic>::PortalType
Storage<T, svtkm::cont::StorageTagBasic>::GetPortal()
{
  auto v = static_cast<T*>(this->Array);
  return PortalType(v, v + this->NumberOfValues);
}

template <typename T>
typename Storage<T, svtkm::cont::StorageTagBasic>::PortalConstType
Storage<T, svtkm::cont::StorageTagBasic>::GetPortalConst() const
{
  auto v = static_cast<T*>(this->Array);
  return PortalConstType(v, v + this->NumberOfValues);
}

template <typename T>
T* Storage<T, svtkm::cont::StorageTagBasic>::GetArray()
{
  return static_cast<T*>(this->Array);
}

template <typename T>
const T* Storage<T, svtkm::cont::StorageTagBasic>::GetArray() const
{
  return static_cast<T*>(this->Array);
}

template <typename T>
svtkm::Pair<T*, void (*)(void*)> Storage<T, svtkm::cont::StorageTagBasic>::StealArray()
{
  svtkm::Pair<T*, void (*)(void*)> result(static_cast<T*>(this->Array), this->DeleteFunction);
  this->DeleteFunction = nullptr;
  return result;
}

} // namespace internal
}
} // namespace svtkm::cont
#endif
