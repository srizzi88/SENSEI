//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleConstant_h
#define svtk_m_cont_ArrayHandleConstant_h

#include <svtkm/cont/ArrayHandleImplicit.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagConstant
{
};

namespace internal
{

template <typename ValueType>
struct SVTKM_ALWAYS_EXPORT ConstantFunctor
{
  SVTKM_EXEC_CONT
  ConstantFunctor(const ValueType& value = ValueType())
    : Value(value)
  {
  }

  SVTKM_EXEC_CONT
  ValueType operator()(svtkm::Id svtkmNotUsed(index)) const { return this->Value; }

private:
  ValueType Value;
};

template <typename T>
using StorageTagConstantSuperclass =
  typename svtkm::cont::ArrayHandleImplicit<ConstantFunctor<T>>::StorageTag;

template <typename T>
struct Storage<T, svtkm::cont::StorageTagConstant> : Storage<T, StorageTagConstantSuperclass<T>>
{
  using Superclass = Storage<T, StorageTagConstantSuperclass<T>>;
  using Superclass::Superclass;
};

template <typename T, typename Device>
struct ArrayTransfer<T, svtkm::cont::StorageTagConstant, Device>
  : ArrayTransfer<T, StorageTagConstantSuperclass<T>, Device>
{
  using Superclass = ArrayTransfer<T, StorageTagConstantSuperclass<T>, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// \brief An array handle with a constant value.
///
/// ArrayHandleConstant is an implicit array handle with a constant value. A
/// constant array handle is constructed by giving a value and an array length.
/// The resulting array is of the given size with each entry the same value
/// given in the constructor. The array is defined implicitly, so there it
/// takes (almost) no memory.
///
template <typename T>
class ArrayHandleConstant : public svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConstant>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleConstant,
                             (ArrayHandleConstant<T>),
                             (svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConstant>));

  SVTKM_CONT
  ArrayHandleConstant(T value, svtkm::Id numberOfValues = 0)
    : Superclass(typename Superclass::PortalConstControl(internal::ConstantFunctor<T>(value),
                                                         numberOfValues))
  {
  }
};

/// make_ArrayHandleConstant is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// array.
///
template <typename T>
svtkm::cont::ArrayHandleConstant<T> make_ArrayHandleConstant(T value, svtkm::Id numberOfValues)
{
  return svtkm::cont::ArrayHandleConstant<T>(value, numberOfValues);
}
}
} // svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandleConstant<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Constant<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConstant>>
  : SerializableTypeString<svtkm::cont::ArrayHandleConstant<T>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandleConstant<T>>
{
private:
  using Type = svtkm::cont::ArrayHandleConstant<T>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetNumberOfValues());
    svtkmdiy::save(bb, obj.GetPortalConstControl().Get(0));
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::Id count = 0;
    svtkmdiy::load(bb, count);

    T value;
    svtkmdiy::load(bb, value);

    obj = svtkm::cont::make_ArrayHandleConstant(value, count);
  }
};

template <typename T>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConstant>>
  : Serialization<svtkm::cont::ArrayHandleConstant<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleConstant_h
