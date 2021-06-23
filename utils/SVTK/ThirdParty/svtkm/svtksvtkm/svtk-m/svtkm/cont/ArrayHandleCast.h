//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleCast_h
#define svtk_m_cont_ArrayHandleCast_h

#include <svtkm/cont/ArrayHandleTransform.h>

#include <svtkm/cont/Logging.h>

#include <svtkm/Range.h>
#include <svtkm/VecTraits.h>

#include <limits>

namespace svtkm
{
namespace cont
{

template <typename SourceT, typename SourceStorage>
struct SVTKM_ALWAYS_EXPORT StorageTagCast
{
};

namespace internal
{

template <typename FromType, typename ToType>
struct SVTKM_ALWAYS_EXPORT Cast
{
// The following operator looks like it should never issue a cast warning because of
// the static_cast (and we don't want it to issue a warning). However, if ToType is
// an object that has a constructor that takes a value that FromType can be cast to,
// that cast can cause a warning. For example, if FromType is svtkm::Float64 and ToType
// is svtkm::Vec<svtkm::Float32, 3>, the static_cast will first implicitly cast the
// Float64 to a Float32 (which causes a warning) before using the static_cast to
// construct the Vec with the Float64. The easiest way around the problem is to
// just disable all conversion warnings here. (The pragmas are taken from those
// used in Types.h for the VecBase class.)
#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif //not using cuda < 8
#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

  SVTKM_EXEC_CONT
  ToType operator()(const FromType& val) const { return static_cast<ToType>(val); }

#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
};

namespace detail
{

template <typename TargetT, typename SourceT, typename SourceStorage, bool... CastFlags>
struct ArrayHandleCastTraits;

template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>
  : ArrayHandleCastTraits<TargetT,
                          SourceT,
                          SourceStorage,
                          std::is_convertible<SourceT, TargetT>::value,
                          std::is_convertible<TargetT, SourceT>::value>
{
};

// Case where the forward cast is invalid, so this array is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage, bool CanCastBackward>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, false, CanCastBackward>
{
  struct StorageSuperclass : svtkm::cont::internal::UndefinedStorage
  {
    using PortalType = svtkm::cont::internal::detail::UndefinedArrayPortal<TargetT>;
    using PortalConstType = svtkm::cont::internal::detail::UndefinedArrayPortal<TargetT>;
  };
};

// Case where the forward cast is valid but the backward cast is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, false>
{
  using StorageTagSuperclass = StorageTagTransform<svtkm::cont::ArrayHandle<SourceT, SourceStorage>,
                                                   svtkm::cont::internal::Cast<SourceT, TargetT>>;
  using StorageSuperclass = svtkm::cont::internal::Storage<TargetT, StorageTagSuperclass>;
  template <typename Device>
  using ArrayTransferSuperclass = ArrayTransfer<TargetT, StorageTagSuperclass, Device>;
};

// Case where both forward and backward casts are valid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, true>
{
  using StorageTagSuperclass = StorageTagTransform<svtkm::cont::ArrayHandle<SourceT, SourceStorage>,
                                                   svtkm::cont::internal::Cast<SourceT, TargetT>,
                                                   svtkm::cont::internal::Cast<TargetT, SourceT>>;
  using StorageSuperclass = svtkm::cont::internal::Storage<TargetT, StorageTagSuperclass>;
  template <typename Device>
  using ArrayTransferSuperclass = ArrayTransfer<TargetT, StorageTagSuperclass, Device>;
};

} // namespace detail

template <typename TargetT, typename SourceT, typename SourceStorage>
struct Storage<TargetT, svtkm::cont::StorageTagCast<SourceT, SourceStorage>>
  : detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>::StorageSuperclass
{
  using Superclass =
    typename detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>::StorageSuperclass;

  using Superclass::Superclass;
};

template <typename TargetT, typename SourceT, typename SourceStorage, typename Device>
struct ArrayTransfer<TargetT, svtkm::cont::StorageTagCast<SourceT, SourceStorage>, Device>
  : detail::ArrayHandleCastTraits<TargetT,
                                  SourceT,
                                  SourceStorage>::template ArrayTransferSuperclass<Device>
{
  using Superclass =
    typename detail::ArrayHandleCastTraits<TargetT,
                                           SourceT,
                                           SourceStorage>::template ArrayTransferSuperclass<Device>;

  using Superclass::Superclass;
};

} // namespace internal

/// \brief Cast the values of an array to the specified type, on demand.
///
/// ArrayHandleCast is a specialization of ArrayHandleTransform. Given an ArrayHandle
/// and a type, it creates a new handle that returns the elements of the array cast
/// to the specified type.
///
template <typename T, typename ArrayHandleType>
class ArrayHandleCast
  : public svtkm::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCast,
    (ArrayHandleCast<T, ArrayHandleType>),
    (svtkm::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>));

  ArrayHandleCast(const svtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                typename ArrayHandleType::StorageTag>& handle)
    : Superclass(typename Superclass::StorageType(handle))
  {
    this->ValidateTypeCast<typename ArrayHandleType::ValueType>();
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCast() {}

private:
  // Log warnings if type cast is valid but lossy:
  template <typename SrcValueType>
  SVTKM_CONT static typename std::enable_if<!std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
#ifdef SVTKM_ENABLE_LOGGING
    using DstValueType = T;
    using SrcComp = typename svtkm::VecTraits<SrcValueType>::BaseComponentType;
    using DstComp = typename svtkm::VecTraits<DstValueType>::BaseComponentType;
    using SrcLimits = std::numeric_limits<SrcComp>;
    using DstLimits = std::numeric_limits<DstComp>;

    const svtkm::Range SrcRange{ SrcLimits::min(), SrcLimits::max() };
    const svtkm::Range DstRange{ DstLimits::min(), DstLimits::max() };

    const bool RangeLoss = (SrcRange.Max > DstRange.Max || SrcRange.Min < DstRange.Min);
    const bool PrecLoss = SrcLimits::digits > DstLimits::digits;

    if (RangeLoss && PrecLoss)
    {
      SVTKM_LOG_F(svtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range and precision.",
                 svtkm::cont::TypeToString<SrcValueType>().c_str(),
                 svtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (RangeLoss)
    {
      SVTKM_LOG_F(svtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range.",
                 svtkm::cont::TypeToString<SrcValueType>().c_str(),
                 svtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (PrecLoss)
    {
      SVTKM_LOG_F(svtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces precision.",
                 svtkm::cont::TypeToString<SrcValueType>().c_str(),
                 svtkm::cont::TypeToString<DstValueType>().c_str());
    }
#endif // Logging
  }

  template <typename SrcValueType>
  SVTKM_CONT static typename std::enable_if<std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
    //no-op if types match
  }
};

namespace detail
{

template <typename CastType, typename OriginalType, typename ArrayType>
struct MakeArrayHandleCastImpl
{
  using ReturnType = svtkm::cont::ArrayHandleCast<CastType, ArrayType>;

  SVTKM_CONT static ReturnType DoMake(const ArrayType& array) { return ReturnType(array); }
};

template <typename T, typename ArrayType>
struct MakeArrayHandleCastImpl<T, T, ArrayType>
{
  using ReturnType = ArrayType;

  SVTKM_CONT static ReturnType DoMake(const ArrayType& array) { return array; }
};

} // namespace detail

/// make_ArrayHandleCast is convenience function to generate an
/// ArrayHandleCast.
///
template <typename T, typename ArrayType>
SVTKM_CONT
  typename detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>::ReturnType
  make_ArrayHandleCast(const ArrayType& array, const T& = T())
{
  SVTKM_IS_ARRAY_HANDLE(ArrayType);
  using MakeImpl = detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>;
  return MakeImpl::DoMake(array);
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

template <typename T, typename AH>
struct SerializableTypeString<svtkm::cont::ArrayHandleCast<T, AH>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Cast<" + SerializableTypeString<T>::Get() + "," +
      SerializableTypeString<typename AH::ValueType>::Get() + "," +
      SerializableTypeString<typename AH::StorageTag>::Get() + ">";
    return name;
  }
};

template <typename T1, typename T2, typename S>
struct SerializableTypeString<svtkm::cont::ArrayHandle<T1, svtkm::cont::StorageTagCast<T2, S>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleCast<T1, svtkm::cont::ArrayHandle<T2, S>>>
{
};
}
} // namespace svtkm::cont

namespace mangled_diy_namespace
{

template <typename TargetT, typename SourceT, typename SourceStorage>
struct Serialization<
  svtkm::cont::ArrayHandle<TargetT, svtkm::cont::StorageTagCast<SourceT, SourceStorage>>>
{
private:
  using BaseType =
    svtkm::cont::ArrayHandle<TargetT, svtkm::cont::StorageTagCast<SourceT, SourceStorage>>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetStorage().GetArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::cont::ArrayHandle<SourceT, SourceStorage> array;
    svtkmdiy::load(bb, array);
    obj = BaseType(array);
  }
};

template <typename TargetT, typename AH>
struct Serialization<svtkm::cont::ArrayHandleCast<TargetT, AH>>
  : Serialization<svtkm::cont::ArrayHandle<
      TargetT,
      svtkm::cont::StorageTagCast<typename AH::ValueType, typename AH::StorageTag>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // svtk_m_cont_ArrayHandleCast_h
