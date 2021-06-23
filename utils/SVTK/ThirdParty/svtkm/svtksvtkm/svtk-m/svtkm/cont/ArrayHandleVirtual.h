//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleVirtual_h
#define svtk_m_cont_ArrayHandleVirtual_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/DeviceAdapterTag.h>

#include <svtkm/cont/StorageVirtual.h>

#include <memory>

namespace svtkm
{
namespace cont
{


template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayHandleVirtual
  : public svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>
{
  using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagVirtual>;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleVirtual,
                             (ArrayHandleVirtual<T>),
                             (svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>));

  ///Construct a valid ArrayHandleVirtual from an existing ArrayHandle
  ///that doesn't derive from ArrayHandleVirtual.
  ///Note left non-explicit to allow:
  ///
  /// std::vector<svtkm::cont::ArrayHandleVirtual<svtkm::Float64>> vectorOfArrays;
  /// //Make basic array.
  /// svtkm::cont::ArrayHandle<svtkm::Float64> basicArray;
  ///  //Fill basicArray...
  /// vectorOfArrays.push_back(basicArray);
  ///
  /// // Make fancy array.
  /// svtkm::cont::ArrayHandleCounting<svtkm::Float64> fancyArray(-1.0, 0.1, ARRAY_SIZE);
  /// vectorOfArrays.push_back(fancyArray);
  template <typename S>
  ArrayHandleVirtual(const svtkm::cont::ArrayHandle<T, S>& ah)
    : Superclass(StorageType(ah))
  {
    using is_base = std::is_base_of<StorageType, S>;
    static_assert(!is_base::value, "Wrong specialization for ArrayHandleVirtual selected");
  }

  /// Returns true if this array matches the type passed in.
  ///
  template <typename ArrayHandleType>
  SVTKM_CONT bool IsType() const
  {
    SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

    //We need to determine if we are checking that `ArrayHandleType`
    //is a virtual array handle since that is an easy check.
    //Or if we have to go ask the storage if they are holding
    //
    using ST = typename ArrayHandleType::StorageTag;
    using is_base = std::is_same<svtkm::cont::StorageTagVirtual, ST>;

    //Determine if the Value type of the virtual and ArrayHandleType
    //are the same. This an easy compile time check, and doesn't need
    // to be done at runtime.
    using VT = typename ArrayHandleType::ValueType;
    using same_value_type = std::is_same<T, VT>;

    return this->IsSameType<ArrayHandleType>(same_value_type{}, is_base{});
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  SVTKM_CONT ArrayHandleType Cast() const
  {
    SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    //We need to determine if we are checking that `ArrayHandleType`
    //is a virtual array handle since that is an easy check.
    //Or if we have to go ask the storage if they are holding
    //
    using ST = typename ArrayHandleType::StorageTag;
    using is_base = std::is_same<svtkm::cont::StorageTagVirtual, ST>;

    //Determine if the Value type of the virtual and ArrayHandleType
    //are the same. This an easy compile time check, and doesn't need
    // to be done at runtime.
    using VT = typename ArrayHandleType::ValueType;
    using same_value_type = std::is_same<T, VT>;

    return this->CastToType<ArrayHandleType>(same_value_type{}, is_base{});
  }

  /// Returns a new instance of an ArrayHandleVirtual with the same storage
  ///
  SVTKM_CONT ArrayHandleVirtual<T> NewInstance() const
  {
    return ArrayHandleVirtual<T>(this->GetStorage().NewInstance());
  }

private:
  template <typename ArrayHandleType>
  inline bool IsSameType(std::false_type, std::true_type) const
  {
    return false;
  }
  template <typename ArrayHandleType>
  inline bool IsSameType(std::false_type, std::false_type) const
  {
    return false;
  }

  template <typename ArrayHandleType>
  inline bool IsSameType(std::true_type svtkmNotUsed(valueTypesMatch),
                         std::true_type svtkmNotUsed(inheritsFromArrayHandleVirtual)) const
  {
    //The type being past has no requirements in being the most derived type
    //so the typeid info won't match but dynamic_cast will work
    auto casted = dynamic_cast<const ArrayHandleType*>(this);
    return casted != nullptr;
  }

  template <typename ArrayHandleType>
  inline bool IsSameType(std::true_type svtkmNotUsed(valueTypesMatch),
                         std::false_type svtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    auto* storage = this->GetStorage().GetStorageVirtual();
    if (!storage)
    {
      return false;
    }
    using S = typename ArrayHandleType::StorageTag;
    return storage->template IsType<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(std::false_type svtkmNotUsed(valueTypesMatch),
                                    std::true_type svtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    SVTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", svtkm::cont::TypeToString<ArrayHandleType>());
    return ArrayHandleType{};
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(std::false_type svtkmNotUsed(valueTypesMatch),
                                    std::false_type svtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    SVTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", svtkm::cont::TypeToString<ArrayHandleType>());
    return ArrayHandleType{};
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(
    std::true_type svtkmNotUsed(valueTypesMatch),
    std::true_type svtkmNotUsed(inheritsFromArrayHandleVirtual)) const
  {
    //The type being passed has no requirements in being the most derived type
    //so the typeid info won't match but dynamic_cast will work
    const ArrayHandleType* derived = dynamic_cast<const ArrayHandleType*>(this);
    if (!derived)
    {
      SVTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
      throwFailedDynamicCast("ArrayHandleVirtual", svtkm::cont::TypeToString<ArrayHandleType>());
    }
    SVTKM_LOG_CAST_SUCC(*this, derived);
    return *derived;
  }

  template <typename ArrayHandleType>
  ArrayHandleType CastToType(std::true_type svtkmNotUsed(valueTypesMatch),
                             std::false_type svtkmNotUsed(notFromArrayHandleVirtual)) const;
};


//=============================================================================
/// A convenience function for creating an ArrayHandleVirtual.
template <typename T, typename S>
SVTKM_CONT svtkm::cont::ArrayHandleVirtual<T> make_ArrayHandleVirtual(
  const svtkm::cont::ArrayHandle<T, S>& ah)
{
  return svtkm::cont::ArrayHandleVirtual<T>(ah);
}

//=============================================================================
// Free function casting helpers

/// Returns true if \c virtHandle matches the type of ArrayHandleType.
///
template <typename ArrayHandleType, typename T>
SVTKM_CONT inline bool IsType(
  const svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>& virtHandle)
{
  return static_cast<svtkm::cont::ArrayHandleVirtual<T>>(virtHandle)
    .template IsType<ArrayHandleType>();
}

/// Returns \c virtHandle cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType, typename T>
SVTKM_CONT inline ArrayHandleType Cast(
  const svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>& virtHandle)
{
  return static_cast<svtkm::cont::ArrayHandleVirtual<T>>(virtHandle)
    .template Cast<ArrayHandleType>();
}
//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandleVirtual<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Virtual<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>>
  : public SerializableTypeString<svtkm::cont::ArrayHandleVirtual<T>>
{
};

#ifndef svtk_m_cont_ArrayHandleVirtual_cxx

#define SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(T)                                                       \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<T, StorageTagVirtual>;               \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandleVirtual<T>;                           \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<T, 2>, StorageTagVirtual>; \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 2>>;             \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<T, 3>, StorageTagVirtual>; \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 3>>;             \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<T, 4>, StorageTagVirtual>; \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 4>>

SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(char);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Int8);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::UInt8);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Int16);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::UInt16);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Int32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::UInt32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Int64);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::UInt64);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Float32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT(svtkm::Float64);

#undef SVTK_M_ARRAY_HANDLE_VIRTUAL_EXPORT

#endif //svtk_m_cont_ArrayHandleVirtual_cxx
}
} //namespace svtkm::cont


#endif //svtk_m_cont_ArrayHandleVirtual_h
