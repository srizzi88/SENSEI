//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleGroupVec_h
#define svtk_m_cont_ArrayHandleGroupVec_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayPortal.h>
#include <svtkm/cont/ErrorBadValue.h>

namespace svtkm
{
namespace exec
{

namespace internal
{

template <typename PortalType, svtkm::IdComponent N_COMPONENTS>
class SVTKM_ALWAYS_EXPORT ArrayPortalGroupVec
{
  using Writable = svtkm::internal::PortalSupportsSets<PortalType>;

public:
  static constexpr svtkm::IdComponent NUM_COMPONENTS = N_COMPONENTS;
  using SourcePortalType = PortalType;

  using ComponentType = typename std::remove_const<typename SourcePortalType::ValueType>::type;
  using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalGroupVec()
    : SourcePortal()
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalGroupVec(const SourcePortalType& sourcePortal)
    : SourcePortal(sourcePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherSourcePortalType>
  SVTKM_EXEC_CONT ArrayPortalGroupVec(
    const ArrayPortalGroupVec<OtherSourcePortalType, NUM_COMPONENTS>& src)
    : SourcePortal(src.GetPortal())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->SourcePortal.GetNumberOfValues() / NUM_COMPONENTS;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    ValueType result;
    svtkm::Id sourceIndex = index * NUM_COMPONENTS;
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      result[componentIndex] = this->SourcePortal.Get(sourceIndex);
      sourceIndex++;
    }
    return result;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    svtkm::Id sourceIndex = index * NUM_COMPONENTS;
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      this->SourcePortal.Set(sourceIndex, value[componentIndex]);
      sourceIndex++;
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const SourcePortalType& GetPortal() const { return this->SourcePortal; }

private:
  SourcePortalType SourcePortal;
};
}
}
} // namespace svtkm::exec::internal

namespace svtkm
{
namespace cont
{

template <typename SourceStorageTag, svtkm::IdComponent NUM_COMPONENTS>
struct SVTKM_ALWAYS_EXPORT StorageTagGroupVec
{
};

namespace internal
{

template <typename ComponentType, svtkm::IdComponent NUM_COMPONENTS, typename SourceStorageTag>
class Storage<svtkm::Vec<ComponentType, NUM_COMPONENTS>,
              svtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>>
{
  using SourceArrayHandleType = svtkm::cont::ArrayHandle<ComponentType, SourceStorageTag>;

public:
  using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;

  using PortalType =
    svtkm::exec::internal::ArrayPortalGroupVec<typename SourceArrayHandleType::PortalControl,
                                              NUM_COMPONENTS>;
  using PortalConstType =
    svtkm::exec::internal::ArrayPortalGroupVec<typename SourceArrayHandleType::PortalConstControl,
                                              NUM_COMPONENTS>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const SourceArrayHandleType& sourceArray)
    : SourceArray(sourceArray)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->SourceArray.GetPortalControl());
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(this->SourceArray.GetPortalConstControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    svtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if (sourceSize % NUM_COMPONENTS != 0)
    {
      throw svtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize / NUM_COMPONENTS;
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->SourceArray.Allocate(numberOfValues * NUM_COMPONENTS);
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    this->SourceArray.Shrink(numberOfValues * NUM_COMPONENTS);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    if (this->Valid)
    {
      this->SourceArray.ReleaseResources();
    }
  }

  // Required for later use in ArrayTransfer class
  SVTKM_CONT
  const SourceArrayHandleType& GetSourceArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->SourceArray;
  }

private:
  SourceArrayHandleType SourceArray;
  bool Valid;
};

template <typename ComponentType,
          svtkm::IdComponent NUM_COMPONENTS,
          typename SourceStorageTag,
          typename Device>
class ArrayTransfer<svtkm::Vec<ComponentType, NUM_COMPONENTS>,
                    svtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>,
                    Device>
{
public:
  using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;

private:
  using StorageTag = svtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

  using SourceArrayHandleType = svtkm::cont::ArrayHandle<ComponentType, SourceStorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::exec::internal::ArrayPortalGroupVec<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::Portal,
    NUM_COMPONENTS>;
  using PortalConstExecution = svtkm::exec::internal::ArrayPortalGroupVec<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    NUM_COMPONENTS>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : SourceArray(storage->GetSourceArray())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    svtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if (sourceSize % NUM_COMPONENTS != 0)
    {
      throw svtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize / NUM_COMPONENTS;
  }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues() % NUM_COMPONENTS != 0)
    {
      throw svtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalConstExecution(this->SourceArray.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues() % NUM_COMPONENTS != 0)
    {
      throw svtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalExecution(this->SourceArray.PrepareForInPlace(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(
      this->SourceArray.PrepareForOutput(numberOfValues * NUM_COMPONENTS, Device()));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    this->SourceArray.Shrink(numberOfValues * NUM_COMPONENTS);
  }

  SVTKM_CONT
  void ReleaseResources() { this->SourceArray.ReleaseResourcesExecution(); }

private:
  SourceArrayHandleType SourceArray;
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors.
///
/// It is sometimes the case that an array is stored such that consecutive
/// entries are meant to form a group. This fancy array handle takes an array
/// of values and a size of groups and then groups the consecutive values
/// stored in a \c Vec.
///
/// For example, if you have an array handle with the six values 0,1,2,3,4,5
/// and give it to a \c ArrayHandleGroupVec with the number of components set
/// to 3, you get an array that looks like it contains two values of \c Vec
/// values of size 3 with the data [0,1,2], [3,4,5].
///
template <typename SourceArrayHandleType, svtkm::IdComponent NUM_COMPONENTS>
class ArrayHandleGroupVec
  : public svtkm::cont::ArrayHandle<
      svtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
      svtkm::cont::StorageTagGroupVec<typename SourceArrayHandleType::StorageTag, NUM_COMPONENTS>>
{
  SVTKM_IS_ARRAY_HANDLE(SourceArrayHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVec,
    (ArrayHandleGroupVec<SourceArrayHandleType, NUM_COMPONENTS>),
    (svtkm::cont::ArrayHandle<
      svtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
      svtkm::cont::StorageTagGroupVec<typename SourceArrayHandleType::StorageTag, NUM_COMPONENTS>>));

  using ComponentType = typename SourceArrayHandleType::ValueType;

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleGroupVec(const SourceArrayHandleType& sourceArray)
    : Superclass(StorageType(sourceArray))
  {
  }
};

/// \c make_ArrayHandleGroupVec is convenience function to generate an
/// ArrayHandleGroupVec. It takes in an ArrayHandle and the number of components
/// (as a specified template parameter), and returns an array handle with
/// consecutive entries grouped in a Vec.
///
template <svtkm::IdComponent NUM_COMPONENTS, typename ArrayHandleType>
SVTKM_CONT svtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS> make_ArrayHandleGroupVec(
  const ArrayHandleType& array)
{
  return svtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS>(array);
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

template <typename AH, svtkm::IdComponent NUM_COMPS>
struct SerializableTypeString<svtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_GroupVec<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NUM_COMPS) + ">";
    return name;
  }
};

template <typename T, svtkm::IdComponent NUM_COMPS, typename ST>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, NUM_COMPS>, svtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : SerializableTypeString<
      svtkm::cont::ArrayHandleGroupVec<svtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, svtkm::IdComponent NUM_COMPS>
struct Serialization<svtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
private:
  using Type = svtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetStorage().GetSourceArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    svtkmdiy::load(bb, array);

    obj = svtkm::cont::make_ArrayHandleGroupVec<NUM_COMPS>(array);
  }
};

template <typename T, svtkm::IdComponent NUM_COMPS, typename ST>
struct Serialization<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, NUM_COMPS>, svtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : Serialization<svtkm::cont::ArrayHandleGroupVec<svtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleGroupVec_h
