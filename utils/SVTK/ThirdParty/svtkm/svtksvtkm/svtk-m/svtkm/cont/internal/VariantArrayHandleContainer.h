//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_VariantArrayHandleContainer_h
#define svtk_m_cont_VariantArrayHandleContainer_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/ArrayHandleVirtual.hxx>
#include <svtkm/cont/ErrorBadType.h>

#include <svtkm/VecTraits.h>

#include <memory>
#include <sstream>
#include <typeindex>

namespace svtkm
{
namespace cont
{

// Forward declaration needed for GetContainer
template <typename TypeList>
class VariantArrayHandleBase;

namespace internal
{

/// \brief Base class for VariantArrayHandleContainer
///
struct SVTKM_CONT_EXPORT VariantArrayHandleContainerBase
{
  std::type_index TypeIndex;

  VariantArrayHandleContainerBase();
  explicit VariantArrayHandleContainerBase(const std::type_info& hash);

  // This must exist so that subclasses are destroyed correctly.
  virtual ~VariantArrayHandleContainerBase();

  virtual svtkm::Id GetNumberOfValues() const = 0;
  virtual svtkm::IdComponent GetNumberOfComponents() const = 0;

  virtual void ReleaseResourcesExecution() = 0;
  virtual void ReleaseResources() = 0;

  virtual void PrintSummary(std::ostream& out) const = 0;

  virtual std::shared_ptr<VariantArrayHandleContainerBase> NewInstance() const = 0;
};

/// \brief ArrayHandle container that can use C++ run-time type information.
///
/// The \c VariantArrayHandleContainer holds ArrayHandle objects
/// (with different template parameters) so that it can polymorphically answer
/// simple questions about the object.
///
template <typename T>
struct SVTKM_ALWAYS_EXPORT VariantArrayHandleContainer final : public VariantArrayHandleContainerBase
{
  svtkm::cont::ArrayHandleVirtual<T> Array;
  mutable svtkm::IdComponent NumberOfComponents = 0;

  VariantArrayHandleContainer()
    : VariantArrayHandleContainerBase(typeid(T))
    , Array()
  {
  }

  VariantArrayHandleContainer(const svtkm::cont::ArrayHandleVirtual<T>& array)
    : VariantArrayHandleContainerBase(typeid(T))
    , Array(array)
  {
  }

  ~VariantArrayHandleContainer<T>() override = default;

  svtkm::Id GetNumberOfValues() const override { return this->Array.GetNumberOfValues(); }

  svtkm::IdComponent GetNumberOfComponents() const override
  {
    // Cache number of components to avoid unnecessary device to host transfers of the array.
    // Also assumes that the number of components is constant accross all elements and
    // throughout the life of the array.
    if (this->NumberOfComponents == 0)
    {
      this->NumberOfComponents =
        this->GetNumberOfComponents(typename svtkm::VecTraits<T>::IsSizeStatic{});
    }
    return this->NumberOfComponents;
  }

  void ReleaseResourcesExecution() override { this->Array.ReleaseResourcesExecution(); }
  void ReleaseResources() override { this->Array.ReleaseResources(); }

  void PrintSummary(std::ostream& out) const override
  {
    svtkm::cont::printSummary_ArrayHandle(this->Array, out);
  }

  std::shared_ptr<VariantArrayHandleContainerBase> NewInstance() const override
  {
    return std::make_shared<VariantArrayHandleContainer<T>>(this->Array.NewInstance());
  }

private:
  svtkm::IdComponent GetNumberOfComponents(VecTraitsTagSizeStatic) const
  {
    return svtkm::VecTraits<T>::NUM_COMPONENTS;
  }

  svtkm::IdComponent GetNumberOfComponents(VecTraitsTagSizeVariable) const
  {
    return (this->Array.GetNumberOfValues() == 0)
      ? 0
      : svtkm::VecTraits<T>::GetNumberOfComponents(this->Array.GetPortalConstControl().Get(0));
  }
};

namespace variant
{

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a VariantArrayHandle from another VariantArrayHandle of any other
// type. Since you cannot partially specialize friendship, use this accessor
// class to get at the internals for the copy constructor.
struct GetContainer
{
  template <typename TypeList>
  SVTKM_CONT static const std::shared_ptr<VariantArrayHandleContainerBase>& Extract(
    const svtkm::cont::VariantArrayHandleBase<TypeList>& src)
  {
    return src.ArrayContainer;
  }
};

template <typename T>
SVTKM_CONT bool IsValueType(const VariantArrayHandleContainerBase* container)
{
  if (container == nullptr)
  { //you can't use typeid on nullptr of polymorphic types
    return false;
  }

  //needs optimizations based on platform. !OSX can use typeid
  return container->TypeIndex == std::type_index(typeid(T));
  // return (nullptr != dynamic_cast<const VariantArrayHandleContainer<T>*>(container));
}

template <typename ArrayHandleType>
SVTKM_CONT inline bool IsType(const VariantArrayHandleContainerBase* container)
{ //container could be nullptr
  using T = typename ArrayHandleType::ValueType;
  if (!IsValueType<T>(container))
  {
    return false;
  }

  const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
  return svtkm::cont::IsType<ArrayHandleType>(derived->Array);
}

template <typename T, typename S>
struct SVTKM_ALWAYS_EXPORT Caster
{
  svtkm::cont::ArrayHandle<T, S> operator()(const VariantArrayHandleContainerBase* container) const
  {
    //This needs to be reworked
    using ArrayHandleType = svtkm::cont::ArrayHandle<T, S>;
    if (!IsValueType<T>(container))
    {
      SVTKM_LOG_CAST_FAIL(container, ArrayHandleType);
      throwFailedDynamicCast(svtkm::cont::TypeToString(container),
                             svtkm::cont::TypeToString<ArrayHandleType>());
    }

    const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
    return svtkm::cont::Cast<svtkm::cont::ArrayHandle<T, S>>(derived->Array);
  }
};

template <typename T>
struct SVTKM_ALWAYS_EXPORT Caster<T, svtkm::cont::StorageTagVirtual>
{
  svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual> operator()(
    const VariantArrayHandleContainerBase* container) const
  {
    if (!IsValueType<T>(container))
    {
      SVTKM_LOG_CAST_FAIL(container, svtkm::cont::ArrayHandleVirtual<T>);
      throwFailedDynamicCast(svtkm::cont::TypeToString(container),
                             svtkm::cont::TypeToString<svtkm::cont::ArrayHandleVirtual<T>>());
    }

    // Technically, this method returns a copy of the \c ArrayHandle. But
    // because \c ArrayHandle acts like a shared pointer, it is valid to
    // do the copy.
    const auto* derived = static_cast<const VariantArrayHandleContainer<T>*>(container);
    SVTKM_LOG_CAST_SUCC(container, derived->Array);
    return derived->Array;
  }
};

template <typename ArrayHandleType>
SVTKM_CONT ArrayHandleType Cast(const VariantArrayHandleContainerBase* container)
{ //container could be nullptr
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  using Type = typename ArrayHandleType::ValueType;
  using Storage = typename ArrayHandleType::StorageTag;
  auto ret = Caster<Type, Storage>{}(container);
  return ArrayHandleType(std::move(ret));
}

struct ForceCastToVirtual
{
  template <typename SrcValueType, typename Storage, typename DstValueType>
  SVTKM_CONT typename std::enable_if<std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const svtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             svtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes match
    output = svtkm::cont::make_ArrayHandleVirtual<DstValueType>(input);
  }

  template <typename SrcValueType, typename Storage, typename DstValueType>
  SVTKM_CONT typename std::enable_if<!std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const svtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             svtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes do not match
    this->ValidateWidthAndCast<SrcValueType, DstValueType>(input, output);
  }

private:
  template <typename S,
            typename D,
            typename InputType,
            svtkm::IdComponent SSize = svtkm::VecTraits<S>::NUM_COMPONENTS,
            svtkm::IdComponent DSize = svtkm::VecTraits<D>::NUM_COMPONENTS>
  SVTKM_CONT typename std::enable_if<SSize == DSize>::type ValidateWidthAndCast(
    const InputType& input,
    svtkm::cont::ArrayHandleVirtual<D>& output) const
  { // number of components match
    auto casted = svtkm::cont::make_ArrayHandleCast<D>(input);
    output = svtkm::cont::make_ArrayHandleVirtual<D>(casted);
  }

  template <typename S,
            typename D,
            svtkm::IdComponent SSize = svtkm::VecTraits<S>::NUM_COMPONENTS,
            svtkm::IdComponent DSize = svtkm::VecTraits<D>::NUM_COMPONENTS>
  SVTKM_CONT typename std::enable_if<SSize != DSize>::type ValidateWidthAndCast(
    const ArrayHandleBase&,
    ArrayHandleBase&) const
  { // number of components do not match
    std::ostringstream str;
    str << "VariantArrayHandle::AsVirtual: Cannot cast from " << svtkm::cont::TypeToString<S>()
        << " to " << svtkm::cont::TypeToString<D>() << "; "
                                                      "number of components must match exactly.";
    throw svtkm::cont::ErrorBadType(str.str());
  }
};
}
}
}
} //namespace svtkm::cont::internal::variant

#endif
