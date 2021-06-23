//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleCounting_h
#define svtk_m_cont_ArrayHandleCounting_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageImplicit.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagCounting
{
};

namespace internal
{

/// \brief An implicit array portal that returns an counting value.
template <class CountingValueType>
class SVTKM_ALWAYS_EXPORT ArrayPortalCounting
{
  using ComponentType = typename svtkm::VecTraits<CountingValueType>::ComponentType;

public:
  using ValueType = CountingValueType;

  SVTKM_EXEC_CONT
  ArrayPortalCounting()
    : Start(0)
    , Step(1)
    , NumberOfValues(0)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalCounting(ValueType start, ValueType step, svtkm::Id numValues)
    : Start(start)
    , Step(step)
    , NumberOfValues(numValues)
  {
  }

  template <typename OtherValueType>
  SVTKM_EXEC_CONT ArrayPortalCounting(const ArrayPortalCounting<OtherValueType>& src)
    : Start(src.Start)
    , Step(src.Step)
    , NumberOfValues(src.NumberOfValues)
  {
  }

  template <typename OtherValueType>
  SVTKM_EXEC_CONT ArrayPortalCounting<ValueType>& operator=(
    const ArrayPortalCounting<OtherValueType>& src)
  {
    this->Start = src.Start;
    this->Step = src.Step;
    this->NumberOfValues = src.NumberOfValues;
    return *this;
  }

  SVTKM_EXEC_CONT
  ValueType GetStart() const { return this->Start; }

  SVTKM_EXEC_CONT
  ValueType GetStep() const { return this->Step; }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(index)));
  }

private:
  ValueType Start;
  ValueType Step;
  svtkm::Id NumberOfValues;
};

template <typename T>
using StorageTagCountingSuperclass =
  svtkm::cont::StorageTagImplicit<internal::ArrayPortalCounting<T>>;

template <typename T>
struct Storage<T, svtkm::cont::StorageTagCounting> : Storage<T, StorageTagCountingSuperclass<T>>
{
  using Superclass = Storage<T, StorageTagCountingSuperclass<T>>;
  using Superclass::Superclass;
};

template <typename T, typename Device>
struct ArrayTransfer<T, svtkm::cont::StorageTagCounting, Device>
  : ArrayTransfer<T, StorageTagCountingSuperclass<T>, Device>
{
  using Superclass = ArrayTransfer<T, StorageTagCountingSuperclass<T>, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// ArrayHandleCounting is a specialization of ArrayHandle. By default it
/// contains a increment value, that is increment for each step between zero
/// and the passed in length
template <typename CountingValueType>
class ArrayHandleCounting
  : public svtkm::cont::ArrayHandle<CountingValueType, svtkm::cont::StorageTagCounting>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCounting,
                             (ArrayHandleCounting<CountingValueType>),
                             (svtkm::cont::ArrayHandle<CountingValueType, StorageTagCounting>));

  SVTKM_CONT
  ArrayHandleCounting(CountingValueType start, CountingValueType step, svtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(start, step, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template <typename CountingValueType>
SVTKM_CONT svtkm::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType start, CountingValueType step, svtkm::Id length)
{
  return svtkm::cont::ArrayHandleCounting<CountingValueType>(start, step, length);
}
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandleCounting<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Counting<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagCounting>>
  : SerializableTypeString<svtkm::cont::ArrayHandleCounting<T>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandleCounting<T>>
{
private:
  using Type = svtkm::cont::ArrayHandleCounting<T>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.GetPortalConstControl();
    svtkmdiy::save(bb, portal.GetStart());
    svtkmdiy::save(bb, portal.GetStep());
    svtkmdiy::save(bb, portal.GetNumberOfValues());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    T start{}, step{};
    svtkm::Id count = 0;

    svtkmdiy::load(bb, start);
    svtkmdiy::load(bb, step);
    svtkmdiy::load(bb, count);

    obj = svtkm::cont::make_ArrayHandleCounting(start, step, count);
  }
};

template <typename T>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagCounting>>
  : Serialization<svtkm::cont::ArrayHandleCounting<T>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleCounting_h
