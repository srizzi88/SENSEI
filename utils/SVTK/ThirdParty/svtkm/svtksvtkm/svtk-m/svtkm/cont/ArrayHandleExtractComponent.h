//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleExtractComponent_h
#define svtk_m_cont_ArrayHandleExtractComponent_h

#include <svtkm/VecTraits.h>
#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename PortalType>
class SVTKM_ALWAYS_EXPORT ArrayPortalExtractComponent
{
  using Writable = svtkm::internal::PortalSupportsSets<PortalType>;

public:
  using VectorType = typename PortalType::ValueType;
  using Traits = svtkm::VecTraits<VectorType>;
  using ValueType = typename Traits::ComponentType;

  SVTKM_EXEC_CONT
  ArrayPortalExtractComponent()
    : Portal()
    , Component(0)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalExtractComponent(const PortalType& portal, svtkm::IdComponent component)
    : Portal(portal)
    , Component(component)
  {
  }

  // Copy constructor
  SVTKM_EXEC_CONT ArrayPortalExtractComponent(const ArrayPortalExtractComponent<PortalType>& src)
    : Portal(src.Portal)
    , Component(src.Component)
  {
  }

  ArrayPortalExtractComponent& operator=(const ArrayPortalExtractComponent& src) = default;
  ArrayPortalExtractComponent& operator=(ArrayPortalExtractComponent&& src) = default;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    return Traits::GetComponent(this->Portal.Get(index), this->Component);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    VectorType vec = this->Portal.Get(index);
    Traits::SetComponent(vec, this->Component, value);
    this->Portal.Set(index, vec);
  }

  SVTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
  svtkm::IdComponent Component;
}; // class ArrayPortalExtractComponent

} // namespace internal

template <typename ArrayHandleType>
class StorageTagExtractComponent
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
              StorageTagExtractComponent<ArrayHandleType>>
{
public:
  using PortalType = ArrayPortalExtractComponent<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalExtractComponent<typename ArrayHandleType::PortalConstControl>;
  using ValueType = typename PortalType::ValueType;

  SVTKM_CONT
  Storage()
    : Array()
    , Component(0)
    , Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& array, svtkm::IdComponent component)
    : Array(array)
    , Component(component)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->Component);
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->Component);
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
  svtkm::IdComponent GetComponent() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Component;
  }

private:
  ArrayHandleType Array;
  svtkm::IdComponent Component;
  bool Valid;
}; // class Storage

template <typename ArrayHandleType, typename Device>
class ArrayTransfer<typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
                    StorageTagExtractComponent<ArrayHandleType>,
                    Device>
{
public:
  using ValueType = typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType;

private:
  using StorageTag = StorageTagExtractComponent<ArrayHandleType>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;
  using ArrayValueType = typename ArrayHandleType::ValueType;
  using ArrayStorageTag = typename ArrayHandleType::StorageTag;
  using ArrayStorageType =
    svtkm::cont::internal::Storage<typename ArrayHandleType::ValueType, ArrayStorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using ExecutionTypes = typename ArrayHandleType::template ExecutionTypes<Device>;
  using PortalExecution = ArrayPortalExtractComponent<typename ExecutionTypes::Portal>;
  using PortalConstExecution = ArrayPortalExtractComponent<typename ExecutionTypes::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Component(storage->GetComponent())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()), this->Component);
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()), this->Component);
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()), this->Component);
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
  svtkm::IdComponent Component;
};
}
}
} // namespace svtkm::cont::internal

namespace svtkm
{
namespace cont
{

/// \brief A fancy ArrayHandle that turns a vector array into a scalar array by
/// slicing out a single component of each vector.
///
/// ArrayHandleExtractComponent is a specialization of ArrayHandle. It takes an
/// input ArrayHandle with a svtkm::Vec ValueType and a component index
/// and uses this information to expose a scalar array consisting of the
/// specified component across all vectors in the input ArrayHandle. So for a
/// given index i, ArrayHandleExtractComponent looks up the i-th svtkm::Vec in
/// the index array and reads or writes to the specified component, leave all
/// other components unmodified. This is done on the fly rather than creating a
/// copy of the array.
template <typename ArrayHandleType>
class ArrayHandleExtractComponent
  : public svtkm::cont::ArrayHandle<
      typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtractComponent,
    (ArrayHandleExtractComponent<ArrayHandleType>),
    (svtkm::cont::ArrayHandle<
      typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>));

protected:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleExtractComponent(const ArrayHandleType& array, svtkm::IdComponent component)
    : Superclass(StorageType(array, component))
  {
  }
};

/// make_ArrayHandleExtractComponent is convenience function to generate an
/// ArrayHandleExtractComponent.
template <typename ArrayHandleType>
SVTKM_CONT ArrayHandleExtractComponent<ArrayHandleType> make_ArrayHandleExtractComponent(
  const ArrayHandleType& array,
  svtkm::IdComponent component)
{
  return ArrayHandleExtractComponent<ArrayHandleType>(array, component);
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

template <typename AH>
struct SerializableTypeString<svtkm::cont::ArrayHandleExtractComponent<AH>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtractComponent<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename AH>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<typename svtkm::VecTraits<typename AH::ValueType>::ComponentType,
                          svtkm::cont::StorageTagExtractComponent<AH>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleExtractComponent<AH>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<svtkm::cont::ArrayHandleExtractComponent<AH>>
{
private:
  using Type = svtkm::cont::ArrayHandleExtractComponent<AH>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetComponent());
    svtkmdiy::save(bb, storage.GetArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::IdComponent component = 0;
    AH array;
    svtkmdiy::load(bb, component);
    svtkmdiy::load(bb, array);

    obj = svtkm::cont::make_ArrayHandleExtractComponent(array, component);
  }
};

template <typename AH>
struct Serialization<
  svtkm::cont::ArrayHandle<typename svtkm::VecTraits<typename AH::ValueType>::ComponentType,
                          svtkm::cont::StorageTagExtractComponent<AH>>>
  : Serialization<svtkm::cont::ArrayHandleExtractComponent<AH>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif // svtk_m_cont_ArrayHandleExtractComponent_h
