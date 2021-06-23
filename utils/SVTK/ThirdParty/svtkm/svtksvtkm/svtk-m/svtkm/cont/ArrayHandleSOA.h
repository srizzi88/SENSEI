//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleSOA_h
#define svtk_m_cont_ArrayHandleSOA_h

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/Math.h>
#include <svtkm/VecTraits.h>

#include <svtkm/internal/ArrayPortalHelpers.h>

#include <svtkmtaotuple/include/tao/seq/make_integer_sequence.hpp>

#include <array>
#include <limits>
#include <type_traits>

namespace svtkm
{

namespace internal
{

/// \brief An array portal that combines indices from multiple sources.
///
/// This will only work if \c VecTraits is defined for the type.
///
template <typename ValueType_, typename SourcePortalType>
class ArrayPortalSOA
{
public:
  using ValueType = ValueType_;

private:
  using ComponentType = typename SourcePortalType::ValueType;

  SVTKM_STATIC_ASSERT(svtkm::HasVecTraits<ValueType>::value);
  using VTraits = svtkm::VecTraits<ValueType>;
  SVTKM_STATIC_ASSERT((std::is_same<typename VTraits::ComponentType, ComponentType>::value));
  static constexpr svtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

  SourcePortalType Portals[NUM_COMPONENTS];
  svtkm::Id NumberOfValues;

public:
  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT explicit ArrayPortalSOA(svtkm::Id numValues = 0)
    : NumberOfValues(numValues)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT void SetPortal(svtkm::IdComponent index, const SourcePortalType& portal)
  {
    this->Portals[index] = portal;
  }

  SVTKM_EXEC_CONT svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  template <typename SPT = SourcePortalType,
            typename Supported = typename svtkm::internal::PortalSupportsGets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  SVTKM_EXEC_CONT ValueType Get(svtkm::Id valueIndex) const
  {
    return this->Get(valueIndex, tao::seq::make_index_sequence<NUM_COMPONENTS>());
  }

  template <typename SPT = SourcePortalType,
            typename Supported = typename svtkm::internal::PortalSupportsSets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id valueIndex, const ValueType& value) const
  {
    this->Set(valueIndex, value, tao::seq::make_index_sequence<NUM_COMPONENTS>());
  }

private:
  template <std::size_t I>
  SVTKM_EXEC_CONT ComponentType GetComponent(svtkm::Id valueIndex) const
  {
    return this->Portals[I].Get(valueIndex);
  }

  template <std::size_t... I>
  SVTKM_EXEC_CONT ValueType Get(svtkm::Id valueIndex, tao::seq::index_sequence<I...>) const
  {
    return ValueType{ this->GetComponent<I>(valueIndex)... };
  }

  template <std::size_t I>
  SVTKM_EXEC_CONT bool SetComponent(svtkm::Id valueIndex, const ValueType& value) const
  {
    this->Portals[I].Set(valueIndex,
                         VTraits::GetComponent(value, static_cast<svtkm::IdComponent>(I)));
    return true;
  }

  template <std::size_t... I>
  SVTKM_EXEC_CONT void Set(svtkm::Id valueIndex,
                          const ValueType& value,
                          tao::seq::index_sequence<I...>) const
  {
    // Is there a better way to unpack an expression and execute them with no other side effects?
    (void)std::initializer_list<bool>{ this->SetComponent<I>(valueIndex, value)... };
  }
};

} // namespace internal

namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagSOA
{
};

namespace internal
{

namespace detail
{

template <typename ValueType, typename PortalType, typename IsTrueVec>
struct SOAPortalChooser;

template <typename ValueType, typename PortalType>
struct SOAPortalChooser<ValueType, PortalType, std::true_type>
{
  using Type = svtkm::internal::ArrayPortalSOA<ValueType, PortalType>;
};

template <typename ValueType, typename PortalType>
struct SOAPortalChooser<ValueType, PortalType, std::false_type>
{
  using Type = PortalType;
};

template <typename ReturnType, typename ValueType, std::size_t NUM_COMPONENTS, typename PortalMaker>
ReturnType MakeSOAPortal(std::array<svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagBasic>,
                                    NUM_COMPONENTS> arrays,
                         svtkm::Id numValues,
                         const PortalMaker& portalMaker)
{
  ReturnType portal(numValues);
  for (std::size_t componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
  {
    portal.SetPortal(static_cast<svtkm::IdComponent>(componentIndex),
                     portalMaker(arrays[componentIndex]));
    SVTKM_ASSERT(arrays[componentIndex].GetNumberOfValues() == numValues);
  }
  return portal;
}

template <typename ReturnType, typename ValueType, typename PortalMaker>
ReturnType MakeSOAPortal(
  std::array<svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagBasic>, 1> arrays,
  svtkm::Id svtkmNotUsed(numValues),
  const PortalMaker& portalMaker)
{
  return portalMaker(arrays[0]);
}

} // namespace detail

template <typename ValueType>
struct ArrayHandleSOATraits
{
  using VTraits = svtkm::VecTraits<ValueType>;
  using ComponentType = typename VTraits::ComponentType;
  using BaseArrayType = svtkm::cont::ArrayHandle<ComponentType, svtkm::cont::StorageTagBasic>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;
  SVTKM_STATIC_ASSERT_MSG(NUM_COMPONENTS > 0,
                         "ArrayHandleSOA requires a type with at least 1 component.");

  using IsTrueVec = std::integral_constant<bool, (NUM_COMPONENTS > 1)>;

  using PortalControl = typename detail::SOAPortalChooser<ValueType,
                                                          typename BaseArrayType::PortalControl,
                                                          IsTrueVec>::Type;
  using PortalConstControl =
    typename detail::SOAPortalChooser<ValueType,
                                      typename BaseArrayType::PortalConstControl,
                                      IsTrueVec>::Type;

  template <typename Device>
  using PortalExecution = typename detail::SOAPortalChooser<
    ValueType,
    typename BaseArrayType::template ExecutionTypes<Device>::Portal,
    IsTrueVec>::Type;
  template <typename Device>
  using PortalConstExecution = typename detail::SOAPortalChooser<
    ValueType,
    typename BaseArrayType::template ExecutionTypes<Device>::PortalConst,
    IsTrueVec>::Type;
};

template <typename ValueType_>
class Storage<ValueType_, svtkm::cont::StorageTagSOA>
{
  using Traits = ArrayHandleSOATraits<ValueType_>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;
  using BaseArrayType = typename Traits::BaseArrayType;

  std::array<BaseArrayType, NUM_COMPONENTS> Arrays;

  SVTKM_CONT bool IsValidImpl(std::true_type) const
  {
    svtkm::Id size = this->Arrays[0].GetNumberOfValues();
    for (svtkm::IdComponent componentIndex = 1; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      if (this->GetArray(componentIndex).GetNumberOfValues() != size)
      {
        return false;
      }
    }
    return true;
  }

  SVTKM_CONT constexpr bool IsValidImpl(std::false_type) const { return true; }

public:
  using ValueType = ValueType_;
  using PortalType = typename Traits::PortalControl;
  using PortalConstType = typename Traits::PortalConstControl;

  SVTKM_CONT bool IsValid() const { return this->IsValidImpl(typename Traits::IsTrueVec{}); }

  Storage() = default;

  SVTKM_CONT Storage(std::initializer_list<BaseArrayType>&& arrays)
    : Arrays{ std::move(arrays) }
  {
    SVTKM_ASSERT(IsValid());
  }

  // For this constructor to work, all types have to be
  // svtkm::cont::ArrayHandle<ValueType, StorageTagBasic>
  template <typename... ArrayTypes>
  SVTKM_CONT Storage(const BaseArrayType& array0, const ArrayTypes&... arrays)
    : Arrays{ { array0, arrays... } }
  {
    SVTKM_ASSERT(IsValid());
  }

  SVTKM_CONT BaseArrayType& GetArray(svtkm::IdComponent index)
  {
    return this->Arrays[static_cast<std::size_t>(index)];
  }

  SVTKM_CONT const BaseArrayType& GetArray(svtkm::IdComponent index) const
  {
    return this->Arrays[static_cast<std::size_t>(index)];
  }

  SVTKM_CONT std::array<BaseArrayType, NUM_COMPONENTS>& GetArrays() { return this->Arrays; }

  SVTKM_CONT const std::array<BaseArrayType, NUM_COMPONENTS>& GetArrays() const
  {
    return this->Arrays;
  }

  SVTKM_CONT void SetArray(svtkm::IdComponent index, const BaseArrayType& array)
  {
    this->Arrays[static_cast<std::size_t>(index)] = array;
  }

  SVTKM_CONT svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(IsValid());
    return this->GetArray(0).GetNumberOfValues();
  }

  SVTKM_CONT PortalType GetPortal()
  {
    SVTKM_ASSERT(this->IsValid());
    return detail::MakeSOAPortal<PortalType>(
      this->Arrays, this->GetNumberOfValues(), [](BaseArrayType& array) {
        return array.GetPortalControl();
      });
  }

  SVTKM_CONT PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->IsValid());
    return detail::MakeSOAPortal<PortalConstType>(
      this->Arrays, this->GetNumberOfValues(), [](const BaseArrayType& array) {
        return array.GetPortalConstControl();
      });
  }

  SVTKM_CONT void Allocate(svtkm::Id numValues)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).Allocate(numValues);
    }
  }

  SVTKM_CONT void Shrink(svtkm::Id numValues)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).Shrink(numValues);
    }
  }

  SVTKM_CONT void ReleaseResources()
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->GetArray(componentIndex).ReleaseResources();
    }
  }
};

template <typename ValueType_, typename Device>
class ArrayTransfer<ValueType_, svtkm::cont::StorageTagSOA, Device>
{
  SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

  using StorageType = svtkm::cont::internal::Storage<ValueType_, svtkm::cont::StorageTagSOA>;

  using Traits = ArrayHandleSOATraits<ValueType_>;
  using BaseArrayType = typename Traits::BaseArrayType;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  StorageType* Storage;

public:
  using ValueType = ValueType_;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = typename Traits::template PortalExecution<Device>;
  using PortalConstExecution = typename Traits::template PortalConstExecution<Device>;

  SVTKM_CONT ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  SVTKM_CONT svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  SVTKM_CONT PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData)) const
  {
    return detail::MakeSOAPortal<PortalConstExecution>(
      this->Storage->GetArrays(), this->GetNumberOfValues(), [](const BaseArrayType& array) {
        return array.PrepareForInput(Device{});
      });
  }

  SVTKM_CONT PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData)) const
  {
    return detail::MakeSOAPortal<PortalExecution>(
      this->Storage->GetArrays(), this->GetNumberOfValues(), [](BaseArrayType& array) {
        return array.PrepareForInPlace(Device{});
      });
  }

  SVTKM_CONT PortalExecution PrepareForOutput(svtkm::Id numValues) const
  {
    return detail::MakeSOAPortal<PortalExecution>(
      this->Storage->GetArrays(), numValues, [numValues](BaseArrayType& array) {
        return array.PrepareForOutput(numValues, Device{});
      });
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT void Shrink(svtkm::Id numValues)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->Storage->GetArray(componentIndex).Shrink(numValues);
    }
  }

  SVTKM_CONT void ReleaseResources()
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      this->Storage->GetArray(componentIndex).ReleaseResourcesExecution();
    }
  }
};

} // namespace internal

/// \brief An \c ArrayHandle that for Vecs stores each component in a separate physical array.
///
/// \c ArrayHandleSOA behaves like a regular \c ArrayHandle (with a basic storage) except that
/// if you specify a \c ValueType of a \c Vec or a \c Vec-like, it will actually store each
/// component in a separate physical array. When data are retrieved from the array, they are
/// reconstructed into \c Vec objects as expected.
///
/// The intention of this array type is to help cover the most common ways data is lain out in
/// memory. Typically, arrays of data are either an "array of structures" like the basic storage
/// where you have a single array of structures (like \c Vec) or a "structure of arrays" where
/// you have an array of a basic type (like \c float) for each component of the data being
/// represented. The\c ArrayHandleSOA makes it easy to cover this second case without creating
/// special types.
///
/// \c ArrayHandleSOA can be constructed from a collection of \c ArrayHandle with basic storage.
/// This allows you to construct \c Vec arrays from components without deep copies.
///
template <typename ValueType_>
class ArrayHandleSOA : public ArrayHandle<ValueType_, svtkm::cont::StorageTagSOA>
{
  using Traits = svtkm::cont::internal::ArrayHandleSOATraits<ValueType_>;
  using ComponentType = typename Traits::ComponentType;
  using BaseArrayType = typename Traits::BaseArrayType;

  using StorageType = svtkm::cont::internal::Storage<ValueType_, svtkm::cont::StorageTagSOA>;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleSOA,
                             (ArrayHandleSOA<ValueType_>),
                             (ArrayHandle<ValueType_, svtkm::cont::StorageTagSOA>));

  ArrayHandleSOA(std::initializer_list<BaseArrayType>&& componentArrays)
    : Superclass(StorageType(std::move(componentArrays)))
  {
  }

  ArrayHandleSOA(std::initializer_list<std::vector<ComponentType>>&& componentVectors)
  {
    SVTKM_ASSERT(componentVectors.size() == Traits::NUM_COMPONENTS);
    svtkm::IdComponent componentIndex = 0;
    for (auto&& vectorIter = componentVectors.begin(); vectorIter != componentVectors.end();
         ++vectorIter)
    {
      // Note, std::vectors that come from std::initializer_list must be copied because the scope
      // of the objects in the initializer list disappears.
      this->SetArray(componentIndex, svtkm::cont::make_ArrayHandle(*vectorIter, svtkm::CopyFlag::On));
      ++componentIndex;
    }
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingVectors>
  ArrayHandleSOA(svtkm::CopyFlag copy,
                 const std::vector<ComponentType>& vector0,
                 const RemainingVectors&... componentVectors)
    : Superclass(StorageType(svtkm::cont::make_ArrayHandle(vector0, copy),
                             svtkm::cont::make_ArrayHandle(componentVectors, copy)...))
  {
    SVTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == Traits::NUM_COMPONENTS);
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingVectors>
  ArrayHandleSOA(const std::vector<ComponentType>& vector0,
                 const RemainingVectors&... componentVectors)
    : Superclass(StorageType(svtkm::cont::make_ArrayHandle(vector0),
                             svtkm::cont::make_ArrayHandle(componentVectors)...))
  {
    SVTKM_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == Traits::NUM_COMPONENTS);
  }

  ArrayHandleSOA(std::initializer_list<const ComponentType*> componentArrays,
                 svtkm::Id length,
                 svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
  {
    SVTKM_ASSERT(componentArrays.size() == Traits::NUM_COMPONENTS);
    svtkm::IdComponent componentIndex = 0;
    for (auto&& vectorIter = componentArrays.begin(); vectorIter != componentArrays.end();
         ++vectorIter)
    {
      this->SetArray(componentIndex, svtkm::cont::make_ArrayHandle(*vectorIter, length, copy));
      ++componentIndex;
    }
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingArrays>
  ArrayHandleSOA(svtkm::Id length,
                 svtkm::CopyFlag copy,
                 const ComponentType* array0,
                 const RemainingArrays&... componentArrays)
    : Superclass(StorageType(svtkm::cont::make_ArrayHandle(array0, length, copy),
                             svtkm::cont::make_ArrayHandle(componentArrays, length, copy)...))
  {
    SVTKM_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == Traits::NUM_COMPONENTS);
  }

  // This only works if all the templated arguments are of type std::vector<ComponentType>.
  template <typename... RemainingArrays>
  ArrayHandleSOA(svtkm::Id length,
                 const ComponentType* array0,
                 const RemainingArrays&... componentArrays)
    : Superclass(StorageType(svtkm::cont::make_ArrayHandle(array0, length),
                             svtkm::cont::make_ArrayHandle(componentArrays, length)...))
  {
    SVTKM_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == Traits::NUM_COMPONENTS);
  }

  SVTKM_CONT BaseArrayType& GetArray(svtkm::IdComponent index)
  {
    return this->GetStorage().GetArray(index);
  }

  SVTKM_CONT const BaseArrayType& GetArray(svtkm::IdComponent index) const
  {
    return this->GetStorage().GetArray(index);
  }

  SVTKM_CONT void SetArray(svtkm::IdComponent index, const BaseArrayType& array)
  {
    this->GetStorage().SetArray(index, array);
  }
};

template <typename ValueType>
SVTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<svtkm::cont::ArrayHandle<typename svtkm::VecTraits<ValueType>::ComponentType,
                                                svtkm::cont::StorageTagBasic>>&& componentArrays)
{
  return ArrayHandleSOA<ValueType>(std::move(componentArrays));
}

template <typename ComponentType, typename... RemainingArrays>
SVTKM_CONT
  ArrayHandleSOA<svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(
    const svtkm::cont::ArrayHandle<ComponentType, svtkm::cont::StorageTagBasic>& componentArray0,
    const RemainingArrays&... componentArrays)
{
  return { componentArray0, componentArrays... };
}

template <typename ValueType>
SVTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<std::vector<typename svtkm::VecTraits<ValueType>::ComponentType>>&&
    componentVectors)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors));
}

// This only works if all the templated arguments are of type std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingVectors>
SVTKM_CONT
  ArrayHandleSOA<svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOA(svtkm::CopyFlag copy,
                      const std::vector<ComponentType>& vector0,
                      const RemainingVectors&... componentVectors)
{
  return ArrayHandleSOA<
    svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>(
    vector0, componentVectors..., copy);
}

template <typename ComponentType, typename... RemainingVectors>
SVTKM_CONT
  ArrayHandleSOA<svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>
  make_ArrayHandleSOA(const std::vector<ComponentType>& vector0,
                      const RemainingVectors&... componentVectors)
{
  return ArrayHandleSOA<
    svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingVectors) + 1)>>(
    vector0, componentVectors...);
}

template <typename ValueType>
SVTKM_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<const typename svtkm::VecTraits<ValueType>::ComponentType*>&&
    componentVectors,
  svtkm::Id length,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors), length, copy);
}

// This only works if all the templated arguments are of type std::vector<ComponentType>.
template <typename ComponentType, typename... RemainingArrays>
SVTKM_CONT
  ArrayHandleSOA<svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(svtkm::Id length,
                      svtkm::CopyFlag copy,
                      const ComponentType* array0,
                      const RemainingArrays*... componentArrays)
{
  return ArrayHandleSOA<
    svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>(
    length, copy, array0, componentArrays...);
}

template <typename ComponentType, typename... RemainingArrays>
SVTKM_CONT
  ArrayHandleSOA<svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>
  make_ArrayHandleSOA(svtkm::Id length,
                      const ComponentType* array0,
                      const RemainingArrays*... componentArrays)
{
  return ArrayHandleSOA<
    svtkm::Vec<ComponentType, svtkm::IdComponent(sizeof...(RemainingArrays) + 1)>>(
    length, array0, componentArrays...);
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

template <typename ValueType>
struct SerializableTypeString<svtkm::cont::ArrayHandleSOA<ValueType>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_SOA<" + SerializableTypeString<ValueType>::Get() + ">";
    return name;
  }
};

template <typename ValueType>
struct SerializableTypeString<svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagSOA>>
  : SerializableTypeString<svtkm::cont::ArrayHandleSOA<ValueType>>
{
};
}
} // namespace svtkm::cont

namespace mangled_diy_namespace
{

template <typename ValueType>
struct Serialization<svtkm::cont::ArrayHandleSOA<ValueType>>
{
  using BaseType = svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagSOA>;
  using Traits = svtkm::cont::internal::ArrayHandleSOATraits<ValueType>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      svtkmdiy::save(bb, obj.GetStorage().GetArray(componentIndex));
    }
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      typename Traits::BaseArrayType componentArray;
      svtkmdiy::load(bb, componentArray);
      obj.GetStorage().SetArray(componentIndex, componentArray);
    }
  }
};

template <typename ValueType>
struct Serialization<svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagSOA>>
  : Serialization<svtkm::cont::ArrayHandleSOA<ValueType>>
{
};

} // namespace mangled_diy_namespace

//=============================================================================
// Precompiled instances

#ifndef svtkm_cont_ArrayHandleSOA_cxx

namespace svtkm
{
namespace cont
{

#define SVTKM_ARRAYHANDLE_SOA_EXPORT(Type)                                                          \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagSOA>;                \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<Type, 2>, StorageTagSOA>;  \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<Type, 3>, StorageTagSOA>;  \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<Type, 4>, StorageTagSOA>;

SVTKM_ARRAYHANDLE_SOA_EXPORT(char)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Int8)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::UInt8)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Int16)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::UInt16)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Int32)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::UInt32)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Int64)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::UInt64)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Float32)
SVTKM_ARRAYHANDLE_SOA_EXPORT(svtkm::Float64)

#undef SVTKM_ARRAYHANDLE_SOA_EXPORT
}
} // namespace svtkm::cont

#endif // !svtkm_cont_ArrayHandleSOA_cxx

#endif //svtk_m_cont_ArrayHandleSOA_h
