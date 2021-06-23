//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleSwizzle_h
#define svtk_m_cont_ArrayHandleSwizzle_h

#include <svtkm/StaticAssert.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadValue.h>

#include <sstream>

namespace svtkm
{
namespace cont
{

template <typename InVecType, svtkm::IdComponent OutVecSize>
struct ResizeVectorType
{
private:
  using ComponentType = typename svtkm::VecTraits<InVecType>::ComponentType;

public:
  using Type = svtkm::Vec<ComponentType, OutVecSize>;
};

template <typename ArrayHandleType, svtkm::IdComponent OutVecSize>
class StorageTagSwizzle
{
};

namespace internal
{

template <typename ArrayHandleType, svtkm::IdComponent OutputSize>
struct ArrayHandleSwizzleTraits;

template <typename V, svtkm::IdComponent C, typename S, svtkm::IdComponent OutSize>
struct ArrayHandleSwizzleTraits<svtkm::cont::ArrayHandle<svtkm::Vec<V, C>, S>, OutSize>
{
  using ComponentType = V;
  static constexpr svtkm::IdComponent InVecSize = C;
  static constexpr svtkm::IdComponent OutVecSize = OutSize;

  SVTKM_STATIC_ASSERT(OutVecSize <= InVecSize);
  static constexpr bool AllCompsUsed = (InVecSize == OutVecSize);

  using InValueType = svtkm::Vec<ComponentType, InVecSize>;
  using OutValueType = svtkm::Vec<ComponentType, OutVecSize>;

  using InStorageTag = S;
  using InArrayHandleType = svtkm::cont::ArrayHandle<InValueType, InStorageTag>;

  using OutStorageTag = svtkm::cont::StorageTagSwizzle<InArrayHandleType, OutVecSize>;
  using OutArrayHandleType = svtkm::cont::ArrayHandle<OutValueType, OutStorageTag>;

  using InStorageType = svtkm::cont::internal::Storage<InValueType, InStorageTag>;
  using OutStorageType = svtkm::cont::internal::Storage<OutValueType, OutStorageTag>;

  using MapType = svtkm::Vec<svtkm::IdComponent, OutVecSize>;

  SVTKM_CONT
  static void ValidateMap(const MapType& map)
  {
    for (svtkm::IdComponent i = 0; i < OutVecSize; ++i)
    {
      if (map[i] < 0 || map[i] >= InVecSize)
      {
        std::ostringstream error;
        error << "Invalid swizzle map: Element " << i << " (" << map[i]
              << ") outside valid range [0, " << InVecSize << ").";
        throw svtkm::cont::ErrorBadValue(error.str());
      }
      for (svtkm::IdComponent j = i + 1; j < OutVecSize; ++j)
      {
        if (map[i] == map[j])
        {
          std::ostringstream error;
          error << "Invalid swizzle map: Repeated element (" << map[i] << ")"
                << " at indices " << i << " and " << j << ".";
          throw svtkm::cont::ErrorBadValue(error.str());
        }
      }
    }
  }

  SVTKM_EXEC_CONT
  static void Swizzle(const InValueType& in, OutValueType& out, const MapType& map)
  {
    for (svtkm::IdComponent i = 0; i < OutSize; ++i)
    {
      out[i] = in[map[i]];
    }
  }

  SVTKM_EXEC_CONT
  static void UnSwizzle(const OutValueType& out, InValueType& in, const MapType& map)
  {
    for (svtkm::IdComponent i = 0; i < OutSize; ++i)
    {
      in[map[i]] = out[i];
    }
  }
};

template <typename PortalType, typename ArrayHandleType, svtkm::IdComponent OutSize>
class SVTKM_ALWAYS_EXPORT ArrayPortalSwizzle
{
  using Traits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using Writable = svtkm::internal::PortalSupportsSets<PortalType>;

public:
  using MapType = typename Traits::MapType;
  using ValueType = typename Traits::OutValueType;

  SVTKM_EXEC_CONT
  ArrayPortalSwizzle()
    : Portal()
    , Map()
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalSwizzle(const PortalType& portal, const MapType& map)
    : Portal(portal)
    , Map(map)
  {
  }

  // Copy constructor
  SVTKM_EXEC_CONT ArrayPortalSwizzle(const ArrayPortalSwizzle& src)
    : Portal(src.GetPortal())
    , Map(src.GetMap())
  {
  }

  ArrayPortalSwizzle& operator=(const ArrayPortalSwizzle& src) = default;
  ArrayPortalSwizzle& operator=(ArrayPortalSwizzle&& src) = default;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    ValueType result;
    Traits::Swizzle(this->Portal.Get(index), result, this->Map);
    return result;
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    if (Traits::AllCompsUsed)
    { // No need to prefetch the value, all values overwritten
      typename Traits::InValueType tmp;
      Traits::UnSwizzle(value, tmp, this->Map);
      this->Portal.Set(index, tmp);
    }
    else
    { // Not all values used -- need to initialize the vector
      typename Traits::InValueType tmp = this->Portal.Get(index);
      Traits::UnSwizzle(value, tmp, this->Map);
      this->Portal.Set(index, tmp);
    }
  }

  SVTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

  SVTKM_EXEC_CONT
  const MapType& GetMap() const { return this->Map; }

private:
  PortalType Portal;
  MapType Map;
};

template <typename ArrayHandleType, svtkm::IdComponent OutSize>
class Storage<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
              svtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>
{
  using Traits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;

public:
  using PortalType =
    ArrayPortalSwizzle<typename ArrayHandleType::PortalControl, ArrayHandleType, OutSize>;
  using PortalConstType =
    ArrayPortalSwizzle<typename ArrayHandleType::PortalConstControl, ArrayHandleType, OutSize>;
  using MapType = typename Traits::MapType;
  using ValueType = typename Traits::OutValueType;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& array, const MapType& map)
    : Array(array)
    , Map(map)
    , Valid(true)
  {
    Traits::ValidateMap(this->Map);
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->Map);
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->Map);
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->Array.Allocate(numberOfValues);
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->Array.Shrink(numberOfValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  SVTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array;
  }

  SVTKM_CONT
  const MapType& GetMap() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Map;
  }

private:
  ArrayHandleType Array;
  MapType Map;
  bool Valid;
};

template <typename ArrayHandleType, svtkm::IdComponent OutSize, typename DeviceTag>
class ArrayTransfer<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
                    svtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>,
                    DeviceTag>
{
  using InExecTypes = typename ArrayHandleType::template ExecutionTypes<DeviceTag>;
  using Traits = ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using StorageType = typename Traits::OutStorageType;
  using MapType = typename Traits::MapType;

  template <typename InPortalT>
  using OutExecType = ArrayPortalSwizzle<InPortalT, ArrayHandleType, OutSize>;

public:
  using ValueType = typename Traits::OutValueType;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = OutExecType<typename InExecTypes::Portal>;
  using PortalConstExecution = OutExecType<typename InExecTypes::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Map(storage->GetMap())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(DeviceTag()), this->Map);
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(DeviceTag()), this->Map);
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, DeviceTag()), this->Map);
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }


private:
  ArrayHandleType Array;
  MapType Map;
};

} // end namespace internal

template <typename ArrayHandleType, svtkm::IdComponent OutSize>
class ArrayHandleSwizzle
  : public ArrayHandle<
      typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
      svtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>
{
public:
  using SwizzleTraits = internal::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;
  using StorageType = typename SwizzleTraits::OutStorageType;
  using MapType = typename SwizzleTraits::MapType;

  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleSwizzle,
    (ArrayHandleSwizzle<ArrayHandleType, OutSize>),
    (ArrayHandle<typename ResizeVectorType<typename ArrayHandleType::ValueType, OutSize>::Type,
                 svtkm::cont::StorageTagSwizzle<ArrayHandleType, OutSize>>));

  SVTKM_CONT
  ArrayHandleSwizzle(const ArrayHandleType& array, const MapType& map)
    : Superclass(StorageType(array, map))
  {
  }
};

template <typename ArrayHandleType, svtkm::IdComponent OutSize>
SVTKM_CONT ArrayHandleSwizzle<ArrayHandleType, OutSize> make_ArrayHandleSwizzle(
  const ArrayHandleType& array,
  const svtkm::Vec<svtkm::IdComponent, OutSize>& map)
{
  return ArrayHandleSwizzle<ArrayHandleType, OutSize>(array, map);
}

template <typename ArrayHandleType, typename... SwizzleIndexTypes>
SVTKM_CONT ArrayHandleSwizzle<ArrayHandleType, svtkm::IdComponent(sizeof...(SwizzleIndexTypes) + 1)>
make_ArrayHandleSwizzle(const ArrayHandleType& array,
                        svtkm::IdComponent swizzleIndex0,
                        SwizzleIndexTypes... swizzleIndices)
{
  return make_ArrayHandleSwizzle(array, svtkm::make_Vec(swizzleIndex0, swizzleIndices...));
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

template <typename AH, svtkm::IdComponent NComps>
struct SerializableTypeString<svtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_Swizzle<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NComps) + ">";
    return name;
  }
};

template <typename AH, svtkm::IdComponent NComps>
struct SerializableTypeString<svtkm::cont::ArrayHandle<
  svtkm::Vec<typename svtkm::VecTraits<typename AH::ValueType>::ComponentType, NComps>,
  svtkm::cont::StorageTagSwizzle<AH, NComps>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, svtkm::IdComponent NComps>
struct Serialization<svtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
private:
  using Type = svtkm::cont::ArrayHandleSwizzle<AH, NComps>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetArray());
    svtkmdiy::save(bb, storage.GetMap());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    svtkmdiy::load(bb, array);
    svtkm::Vec<svtkm::IdComponent, NComps> map;
    svtkmdiy::load(bb, map);
    obj = svtkm::cont::make_ArrayHandleSwizzle(array, map);
  }
};

template <typename AH, svtkm::IdComponent NComps>
struct Serialization<svtkm::cont::ArrayHandle<
  svtkm::Vec<typename svtkm::VecTraits<typename AH::ValueType>::ComponentType, NComps>,
  svtkm::cont::StorageTagSwizzle<AH, NComps>>>
  : Serialization<svtkm::cont::ArrayHandleSwizzle<AH, NComps>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // svtk_m_cont_ArrayHandleSwizzle_h
