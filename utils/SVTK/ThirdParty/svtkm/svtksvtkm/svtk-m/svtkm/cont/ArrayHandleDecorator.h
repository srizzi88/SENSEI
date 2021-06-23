//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_ArrayHandleDecorator_h
#define svtk_m_ArrayHandleDecorator_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/Storage.h>

#include <svtkm/StaticAssert.h>
#include <svtkm/VecTraits.h>

#include <svtkm/internal/ArrayPortalHelpers.h>
#include <svtkm/internal/brigand.hpp>

#include <svtkmtaotuple/include/Tuple.h>
#include <svtkmtaotuple/include/tao/seq/make_integer_sequence.hpp>


#include <type_traits>
#include <utility>

// Some compilers like brigand's integer sequences.
// Some compilers prefer tao's.
// Brigand seems to have more support, so we'll use that as default and fallback
// to tao when brigand fails. With C++14, we'll be able to just use the STL.
#if !defined(SVTKM_CUDA_DEVICE_PASS) &&                                                             \
  (defined(SVTKM_GCC) ||                                                                            \
   (defined(__apple_build_version__) && (__apple_build_version__ >= 10000000)) ||                  \
   (defined(SVTKM_CLANG) && (__clang_major__ >= 5)))
#define SVTKM_USE_TAO_SEQ
#endif

namespace svtkm
{
namespace cont
{
namespace internal
{

namespace decor
{

// Generic InverseFunctor implementation that does nothing.
struct NoOpInverseFunctor
{
  NoOpInverseFunctor() = default;
  template <typename... Ts>
  SVTKM_EXEC_CONT NoOpInverseFunctor(Ts...)
  {
  }
  template <typename VT>
  SVTKM_EXEC_CONT void operator()(svtkm::Id, VT) const
  {
  }
};

} // namespace decor

// The portal for ArrayHandleDecorator. Get calls FunctorType::operator(), and
// Set calls InverseFunctorType::operator(), but only if the DecoratorImpl
// provides an inverse.
template <typename ValueType_, typename FunctorType_, typename InverseFunctorType_>
class SVTKM_ALWAYS_EXPORT ArrayPortalDecorator
{
public:
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;
  using InverseFunctorType = InverseFunctorType_;
  using ReadOnly = std::is_same<InverseFunctorType, decor::NoOpInverseFunctor>;

  SVTKM_EXEC_CONT
  ArrayPortalDecorator() {}

  SVTKM_CONT
  ArrayPortalDecorator(FunctorType func, InverseFunctorType iFunc, svtkm::Id numValues)
    : Functor(func)
    , InverseFunctor(iFunc)
    , NumberOfValues(numValues)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return this->Functor(index); }

  template <typename ReadOnly_ = ReadOnly,
            typename = typename std::enable_if<!ReadOnly_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    this->InverseFunctor(index, value);
  }

private:
  FunctorType Functor;
  InverseFunctorType InverseFunctor;
  svtkm::Id NumberOfValues;
};

namespace decor
{

// Ensures that all types in variadic container ArrayHandleList are subclasses
// of ArrayHandleBase.
template <typename ArrayHandleList>
using AllAreArrayHandles =
  brigand::all<ArrayHandleList, std::is_base_of<ArrayHandleBase, brigand::_1>>;

namespace detail
{

// Tests whether DecoratorImplT has a CreateInverseFunctor(Portals...) method.
template <typename DecoratorImplT, typename PortalList>
struct IsFunctorInvertibleImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct IsFunctorInvertibleImpl<DecoratorImplT, List<PortalTs...>>
{
private:
  template <
    typename T,
    typename U = decltype(std::declval<T>().CreateInverseFunctor(std::declval<PortalTs&&>()...))>
  static std::true_type InverseExistsTest(int);

  template <typename T>
  static std::false_type InverseExistsTest(...);

public:
  using type = decltype(InverseExistsTest<DecoratorImplT>(0));
};

// Tests whether DecoratorImplT has an AllocateSourceArrays(size, Arrays...) method.
template <typename DecoratorImplT, typename ArrayList>
struct IsDecoratorAllocatableImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... ArrayTs>
struct IsDecoratorAllocatableImpl<DecoratorImplT, List<ArrayTs...>>
{
private:
  template <
    typename T,
    typename U = decltype(std::declval<T>().AllocateSourceArrays(0, std::declval<ArrayTs&>()...))>
  static std::true_type Exists(int);
  template <typename T>
  static std::false_type Exists(...);

public:
  using type = decltype(Exists<DecoratorImplT>(0));
};

// Tests whether DecoratorImplT has a ShrinkSourceArrays(size, Arrays...) method.
template <typename DecoratorImplT, typename ArrayList>
struct IsDecoratorShrinkableImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... ArrayTs>
struct IsDecoratorShrinkableImpl<DecoratorImplT, List<ArrayTs...>>
{
private:
  template <
    typename T,
    typename U = decltype(std::declval<T>().ShrinkSourceArrays(0, std::declval<ArrayTs&>()...))>
  static std::true_type Exists(int);
  template <typename T>
  static std::false_type Exists(...);

public:
  using type = decltype(Exists<DecoratorImplT>(0));
};

// Deduces the type returned by DecoratorImplT::CreateFunctor when given
// the specified portals.
template <typename DecoratorImplT, typename PortalList>
struct GetFunctorTypeImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct GetFunctorTypeImpl<DecoratorImplT, List<PortalTs...>>
{
  using type =
    decltype(std::declval<DecoratorImplT>().CreateFunctor(std::declval<PortalTs&&>()...));
};

// Deduces the type returned by DecoratorImplT::CreateInverseFunctor when given
// the specified portals. If DecoratorImplT doesn't have a CreateInverseFunctor
// method, a NoOp functor will be used instead.
template <typename CanWrite, typename DecoratorImplT, typename PortalList>
struct GetInverseFunctorTypeImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct GetInverseFunctorTypeImpl<std::true_type, DecoratorImplT, List<PortalTs...>>
{
  using type =
    decltype(std::declval<DecoratorImplT>().CreateInverseFunctor(std::declval<PortalTs&&>()...));
};

template <typename DecoratorImplT, typename PortalList>
struct GetInverseFunctorTypeImpl<std::false_type, DecoratorImplT, PortalList>
{
  using type = NoOpInverseFunctor;
};

// Get appropriate portals from a source array.
// See note below about using non-writable portals in invertible functors.
// We need to sub in const portals when writable ones don't exist.
template <typename ArrayT>
typename std::decay<ArrayT>::type::PortalControl GetPortalControlImpl(std::true_type,
                                                                      ArrayT&& array)
{
  return array.GetPortalControl();
}

template <typename ArrayT>
typename std::decay<ArrayT>::type::PortalConstControl GetPortalControlImpl(std::false_type,
                                                                           ArrayT&& array)
{
  return array.GetPortalConstControl();
}

template <typename ArrayT, typename Device>
typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::Portal
GetPortalInPlaceImpl(std::true_type, ArrayT&& array, Device)
{
  return array.PrepareForInPlace(Device{});
}

template <typename ArrayT, typename Device>
typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::PortalConst
GetPortalInPlaceImpl(std::false_type, ArrayT&& array, Device)
{
  // ArrayT is read-only -- prepare for input instead.
  return array.PrepareForInput(Device{});
}

template <typename ArrayT, typename Device>
typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::Portal
GetPortalOutputImpl(std::true_type, ArrayT&& array, Device)
{
  // Prepare these for inplace usage instead -- we'll likely need to read
  // from these in addition to writing.
  return array.PrepareForInPlace(Device{});
}

template <typename ArrayT, typename Device>
typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::PortalConst
GetPortalOutputImpl(std::false_type, ArrayT&& array, Device)
{
  // ArrayT is read-only -- prepare for input instead.
  return array.PrepareForInput(Device{});
}

} // namespace detail

// Get portal types:
// We allow writing to an AHDecorator if *any* of the ArrayHandles are writable.
// This means we have to avoid calling PrepareForOutput, etc on non-writable
// array handles, since these may throw. On non-writable handles, use the
// const array handles so we can at least read from them in the inverse
// functors.
template <typename ArrayT,
          typename Portal = typename std::decay<ArrayT>::type::PortalControl,
          typename PortalConst = typename std::decay<ArrayT>::type::PortalConstControl>
using GetPortalControlType =
  typename brigand::if_<svtkm::internal::PortalSupportsSets<Portal>, Portal, PortalConst>::type;

template <typename ArrayT>
using GetPortalConstControlType = typename std::decay<ArrayT>::type::PortalConstControl;

template <typename ArrayT,
          typename Device,
          typename Portal =
            typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::Portal,
          typename PortalConst =
            typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::PortalConst>
using GetPortalExecutionType =
  typename brigand::if_<svtkm::internal::PortalSupportsSets<Portal>, Portal, PortalConst>::type;

template <typename ArrayT, typename Device>
using GetPortalConstExecutionType =
  typename std::decay<ArrayT>::type::template ExecutionTypes<Device>::PortalConst;

// Get portal objects:
// See note above -- we swap in const portals sometimes.
template <typename ArrayT>
GetPortalControlType<typename std::decay<ArrayT>::type> GetPortalControl(ArrayT&& array)
{
  return detail::GetPortalControlImpl(IsWritableArrayHandle<ArrayT>{}, std::forward<ArrayT>(array));
}

template <typename ArrayT>
GetPortalConstControlType<typename std::decay<ArrayT>::type> GetPortalConstControl(
  const ArrayT& array)
{
  return array.GetPortalConstControl();
}

template <typename ArrayT, typename Device>
GetPortalConstExecutionType<typename std::decay<ArrayT>::type, Device> GetPortalInput(
  const ArrayT& array,
  Device)
{
  return array.PrepareForInput(Device{});
}

template <typename ArrayT, typename Device>
GetPortalExecutionType<typename std::decay<ArrayT>::type, Device> GetPortalInPlace(ArrayT&& array,
                                                                                   Device)
{
  return detail::GetPortalInPlaceImpl(
    IsWritableArrayHandle<ArrayT>{}, std::forward<ArrayT>(array), Device{});
}

template <typename ArrayT, typename Device>
GetPortalExecutionType<typename std::decay<ArrayT>::type, Device> GetPortalOutput(ArrayT&& array,
                                                                                  Device)
{
  return detail::GetPortalOutputImpl(
    IsWritableArrayHandle<ArrayT>{}, std::forward<ArrayT>(array), Device{});
}

// Equivalent to std::true_type if *any* portal in PortalList can be written to.
// If all are read-only, std::false_type is used instead.
template <typename PortalList>
using AnyPortalIsWritable =
  typename brigand::any<PortalList,
                        brigand::bind<svtkm::internal::PortalSupportsSets, brigand::_1>>::type;

// Set to std::true_type if DecoratorImplT::CreateInverseFunctor can be called
// with the supplied portals, or std::false_type otherwise.
template <typename DecoratorImplT, typename PortalList>
using IsFunctorInvertible =
  typename detail::IsFunctorInvertibleImpl<DecoratorImplT, PortalList>::type;

// Set to std::true_type if DecoratorImplT::AllocateSourceArrays can be called
// with the supplied arrays, or std::false_type otherwise.
template <typename DecoratorImplT, typename ArrayList>
using IsDecoratorAllocatable =
  typename detail::IsDecoratorAllocatableImpl<DecoratorImplT, ArrayList>::type;

// Set to std::true_type if DecoratorImplT::ShrinkSourceArrays can be called
// with the supplied arrays, or std::false_type otherwise.
template <typename DecoratorImplT, typename ArrayList>
using IsDecoratorShrinkable =
  typename detail::IsDecoratorShrinkableImpl<DecoratorImplT, ArrayList>::type;

// std::true_type/std::false_type depending on whether the decorator impl has a
// CreateInversePortal method AND any of the arrays are writable.
template <typename DecoratorImplT, typename PortalList>
using CanWriteToFunctor = typename brigand::and_<IsFunctorInvertible<DecoratorImplT, PortalList>,
                                                 AnyPortalIsWritable<PortalList>>::type;

// The FunctorType for the provided implementation and portal types.
template <typename DecoratorImplT, typename PortalList>
using GetFunctorType = typename detail::GetFunctorTypeImpl<DecoratorImplT, PortalList>::type;

// The InverseFunctorType for the provided implementation and portal types.
// Will detect when inversion is not possible and return a NoOp functor instead.
template <typename DecoratorImplT, typename PortalList>
using GetInverseFunctorType =
  typename detail::GetInverseFunctorTypeImpl<CanWriteToFunctor<DecoratorImplT, PortalList>,
                                             DecoratorImplT,
                                             PortalList>::type;

// Convert a sequence of array handle types to a list of portals:

// Some notes on this implementation:
// - MSVC 2015 ICEs when using brigand::transform to convert a brigand::list
//   of arrayhandles to portals. So instead we pass the ArrayTs.
// - Just using brigand::list<GetPortalControlType<ArrayTs>...> fails, as
//   apparently that is an improper parameter pack expansion
// - So we jump through some decltype/declval hoops here to get this to work:
template <typename... ArrayTs>
using GetPortalConstControlList =
  brigand::list<decltype((GetPortalConstControl(std::declval<ArrayTs&>())))...>;

template <typename Device, typename... ArrayTs>
using GetPortalConstExecutionList =
  brigand::list<decltype((GetPortalInput(std::declval<ArrayTs&>(), Device{})))...>;

template <typename... ArrayTs>
using GetPortalControlList =
  brigand::list<decltype((GetPortalControl(std::declval<ArrayTs&>())))...>;

template <typename Device, typename... ArrayTs>
using GetPortalExecutionList =
  brigand::list<decltype((GetPortalInPlace(std::declval<ArrayTs&>(), Device{})))...>;

template <typename DecoratorImplT, typename... ArrayTs>
struct DecoratorStorageTraits
{
  using ArrayList = brigand::list<ArrayTs...>;

  SVTKM_STATIC_ASSERT_MSG(sizeof...(ArrayTs) > 0,
                         "Must specify at least one source array handle for "
                         "ArrayHandleDecorator. Consider using "
                         "ArrayHandleImplicit instead.");

  // Need to check this here, since this traits struct is used in the
  // ArrayHandleDecorator superclass definition before any other
  // static_asserts could be used.
  SVTKM_STATIC_ASSERT_MSG(decor::AllAreArrayHandles<ArrayList>::value,
                         "Trailing template parameters for "
                         "ArrayHandleDecorator must be a list of ArrayHandle "
                         "types.");

  using ArrayTupleType = svtkmstd::tuple<ArrayTs...>;

// size_t integral constants that index ArrayTs:
#ifndef SVTKM_USE_TAO_SEQ
  using IndexList = brigand::make_sequence<brigand::size_t<0>, sizeof...(ArrayTs)>;
#else  // SVTKM_USE_TAO_SEQ
  using IndexList = tao::seq::make_index_sequence<sizeof...(ArrayTs)>;
#endif // SVTKM_USE_TAO_SEQ

  // true_type/false_type depending on whether the decorator supports Allocate/Shrink:
  using IsAllocatable = IsDecoratorAllocatable<DecoratorImplT, ArrayList>;
  using IsShrinkable = IsDecoratorShrinkable<DecoratorImplT, ArrayList>;

  // Portal lists:
  // NOTE we have to pass the parameter pack here instead of using ArrayList
  // with brigand::transform, since that's causing MSVC 2015 to ice:
  using PortalControlList = GetPortalControlList<ArrayTs...>;
  using PortalConstControlList = GetPortalConstControlList<ArrayTs...>;
  template <typename Device>
  using PortalExecutionList = GetPortalExecutionList<Device, ArrayTs...>;
  template <typename Device>
  using PortalConstExecutionList = GetPortalConstExecutionList<Device, ArrayTs...>;

  // Functors:
  using FunctorControlType = GetFunctorType<DecoratorImplT, PortalControlList>;
  using FunctorConstControlType = GetFunctorType<DecoratorImplT, PortalConstControlList>;

  template <typename Device>
  using FunctorExecutionType = GetFunctorType<DecoratorImplT, PortalExecutionList<Device>>;

  template <typename Device>
  using FunctorConstExecutionType =
    GetFunctorType<DecoratorImplT, PortalConstExecutionList<Device>>;

  // Inverse functors:
  using InverseFunctorControlType = GetInverseFunctorType<DecoratorImplT, PortalControlList>;

  using InverseFunctorConstControlType = NoOpInverseFunctor;

  template <typename Device>
  using InverseFunctorExecutionType =
    GetInverseFunctorType<DecoratorImplT, PortalExecutionList<Device>>;

  template <typename Device>
  using InverseFunctorConstExecutionType = NoOpInverseFunctor;

  // Misc:
  // ValueType is derived from DecoratorImplT::CreateFunctor(...)'s operator().
  using ValueType = decltype(std::declval<FunctorControlType>()(0));

  // Decorator portals:
  using PortalControlType =
    ArrayPortalDecorator<ValueType, FunctorControlType, InverseFunctorControlType>;

  using PortalConstControlType =
    ArrayPortalDecorator<ValueType, FunctorConstControlType, InverseFunctorConstControlType>;

  template <typename Device>
  using PortalExecutionType = ArrayPortalDecorator<ValueType,
                                                   FunctorExecutionType<Device>,
                                                   InverseFunctorExecutionType<Device>>;

  template <typename Device>
  using PortalConstExecutionType = ArrayPortalDecorator<ValueType,
                                                        FunctorConstExecutionType<Device>,
                                                        InverseFunctorConstExecutionType<Device>>;

  // helper for constructing portals with the appropriate functors. This is
  // where we decide whether or not to call `CreateInverseFunctor` on the
  // implementation class.
  // Do not use these directly, they are helpers for the MakePortal[...]
  // methods below.
  template <typename DecoratorPortalType, typename... PortalTs>
  SVTKM_CONT static
    typename std::enable_if<DecoratorPortalType::ReadOnly::value, DecoratorPortalType>::type
    CreatePortalDecorator(svtkm::Id numVals, const DecoratorImplT& impl, PortalTs&&... portals)
  { // Portal is read only:
    return { impl.CreateFunctor(std::forward<PortalTs>(portals)...),
             typename DecoratorPortalType::InverseFunctorType{},
             numVals };
  }

  template <typename DecoratorPortalType, typename... PortalTs>
  SVTKM_CONT static
    typename std::enable_if<!DecoratorPortalType::ReadOnly::value, DecoratorPortalType>::type
    CreatePortalDecorator(svtkm::Id numVals, const DecoratorImplT& impl, PortalTs... portals)
  { // Portal is read/write:
    return { impl.CreateFunctor(portals...), impl.CreateInverseFunctor(portals...), numVals };
  }

  // Static dispatch for calling AllocateSourceArrays on supported implementations:
  SVTKM_CONT[[noreturn]] static void CallAllocate(std::false_type,
                                                 const DecoratorImplT&,
                                                 svtkm::Id,
                                                 ArrayTs&...)
  {
    throw svtkm::cont::ErrorBadType("Allocate not supported by this ArrayHandleDecorator.");
  }

  SVTKM_CONT static void CallAllocate(std::true_type,
                                     const DecoratorImplT& impl,
                                     svtkm::Id newSize,
                                     ArrayTs&... arrays)
  {
    impl.AllocateSourceArrays(newSize, arrays...);
  }

  // Static dispatch for calling ShrinkSourceArrays on supported implementations.
  SVTKM_CONT[[noreturn]] static void CallShrink(std::false_type,
                                               const DecoratorImplT&,
                                               svtkm::Id,
                                               ArrayTs&...)
  {
    throw svtkm::cont::ErrorBadType("Shrink not supported by this ArrayHandleDecorator.");
  }

  SVTKM_CONT static void CallShrink(std::true_type,
                                   const DecoratorImplT& impl,
                                   svtkm::Id newSize,
                                   ArrayTs&... arrays)
  {
    impl.ShrinkSourceArrays(newSize, arrays...);
  }


#ifndef SVTKM_USE_TAO_SEQ
  // Portal construction methods. These actually create portals.
  template <template <typename...> class List, typename... Indices>
  SVTKM_CONT static PortalControlType MakePortalControl(const DecoratorImplT& impl,
                                                       ArrayTupleType& arrays,
                                                       svtkm::Id numValues,
                                                       List<Indices...>)
  {
    return CreatePortalDecorator<PortalControlType>(
      numValues,
      impl,
      // More MSVC ICE avoidance while expanding the Indices parameter pack,
      // which is a list of std::integral_constant<size_t, ...>:
      //
      // Indices::value : Works everywhere but MSVC2017, which crashes on it.
      // Indices{} : Works on MSVC2017, but won't compile on MSVC2015.
      // Indices{}.value : Works on both MSVC2015 and MSVC2017.
      //
      // Don't touch the following line unless you really, really have to.
      GetPortalControl(svtkmstd::get<Indices{}.value>(arrays))...);
  }

  template <template <typename...> class List, typename... Indices>
  SVTKM_CONT static PortalConstControlType MakePortalConstControl(const DecoratorImplT& impl,
                                                                 const ArrayTupleType& arrays,
                                                                 svtkm::Id numValues,
                                                                 List<Indices...>)
  {
    return CreatePortalDecorator<PortalConstControlType>(
      numValues,
      impl,
      // Don't touch the following line unless you really, really have to. See
      // note in MakePortalControl.
      GetPortalConstControl(svtkmstd::get<Indices{}.value>(arrays))...);
  }

  template <template <typename...> class List, typename... Indices, typename Device>
  SVTKM_CONT static PortalConstExecutionType<Device> MakePortalInput(const DecoratorImplT& impl,
                                                                    const ArrayTupleType& arrays,
                                                                    svtkm::Id numValues,
                                                                    List<Indices...>,
                                                                    Device dev)
  {
    return CreatePortalDecorator<PortalConstExecutionType<Device>>(
      numValues,
      impl,
      // Don't touch the following line unless you really, really have to. See
      // note in MakePortalControl.
      GetPortalInput(svtkmstd::get<Indices{}.value>(arrays), dev)...);
  }

  template <template <typename...> class List, typename... Indices, typename Device>
  SVTKM_CONT static PortalExecutionType<Device> MakePortalInPlace(const DecoratorImplT& impl,
                                                                 ArrayTupleType& arrays,
                                                                 svtkm::Id numValues,
                                                                 List<Indices...>,
                                                                 Device dev)
  {
    return CreatePortalDecorator<PortalExecutionType<Device>>(
      numValues,
      impl,
      // Don't touch the following line unless you really, really have to. See
      // note in MakePortalControl.
      GetPortalInPlace(svtkmstd::get<Indices{}.value>(arrays), dev)...);
  }

  template <template <typename...> class List, typename... Indices, typename Device>
  SVTKM_CONT static PortalExecutionType<Device> MakePortalOutput(const DecoratorImplT& impl,
                                                                ArrayTupleType& arrays,
                                                                svtkm::Id numValues,
                                                                List<Indices...>,
                                                                Device dev)
  {
    return CreatePortalDecorator<PortalExecutionType<Device>>(
      numValues,
      impl,
      // Don't touch the following line unless you really, really have to. See
      // note in MakePortalControl.
      GetPortalOutput(svtkmstd::get<Indices{}.value>(arrays), dev)...);
  }

  template <template <typename...> class List, typename... Indices>
  SVTKM_CONT static void AllocateSourceArrays(const DecoratorImplT& impl,
                                             ArrayTupleType& arrays,
                                             svtkm::Id numValues,
                                             List<Indices...>)
  {
    CallAllocate(IsAllocatable{}, impl, numValues, svtkmstd::get<Indices{}.value>(arrays)...);
  }

  template <template <typename...> class List, typename... Indices>
  SVTKM_CONT static void ShrinkSourceArrays(const DecoratorImplT& impl,
                                           ArrayTupleType& arrays,
                                           svtkm::Id numValues,
                                           List<Indices...>)
  {
    CallShrink(IsShrinkable{}, impl, numValues, svtkmstd::get<Indices{}.value>(arrays)...);
  }

#else // SVTKM_USE_TAO_SEQ

  // Portal construction methods. These actually create portals.
  template <template <typename, std::size_t...> class List, std::size_t... Indices>
  SVTKM_CONT static PortalControlType MakePortalControl(const DecoratorImplT& impl,
                                                       ArrayTupleType& arrays,
                                                       svtkm::Id numValues,
                                                       List<std::size_t, Indices...>)
  {
    return CreatePortalDecorator<PortalControlType>(
      numValues, impl, GetPortalControl(svtkmstd::get<Indices>(arrays))...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices>
  SVTKM_CONT static PortalConstControlType MakePortalConstControl(const DecoratorImplT& impl,
                                                                 const ArrayTupleType& arrays,
                                                                 svtkm::Id numValues,
                                                                 List<std::size_t, Indices...>)
  {
    return CreatePortalDecorator<PortalConstControlType>(
      numValues, impl, GetPortalConstControl(svtkmstd::get<Indices>(arrays))...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices, typename Device>
  SVTKM_CONT static PortalConstExecutionType<Device> MakePortalInput(const DecoratorImplT& impl,
                                                                    const ArrayTupleType& arrays,
                                                                    svtkm::Id numValues,
                                                                    List<std::size_t, Indices...>,
                                                                    Device dev)
  {
    return CreatePortalDecorator<PortalConstExecutionType<Device>>(
      numValues, impl, GetPortalInput(svtkmstd::get<Indices>(arrays), dev)...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices, typename Device>
  SVTKM_CONT static PortalExecutionType<Device> MakePortalInPlace(const DecoratorImplT& impl,
                                                                 ArrayTupleType& arrays,
                                                                 svtkm::Id numValues,
                                                                 List<std::size_t, Indices...>,
                                                                 Device dev)
  {
    return CreatePortalDecorator<PortalExecutionType<Device>>(
      numValues, impl, GetPortalInPlace(svtkmstd::get<Indices>(arrays), dev)...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices, typename Device>
  SVTKM_CONT static PortalExecutionType<Device> MakePortalOutput(const DecoratorImplT& impl,
                                                                ArrayTupleType& arrays,
                                                                svtkm::Id numValues,
                                                                List<std::size_t, Indices...>,
                                                                Device dev)
  {
    return CreatePortalDecorator<PortalExecutionType<Device>>(
      numValues, impl, GetPortalOutput(svtkmstd::get<Indices>(arrays), dev)...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices>
  SVTKM_CONT static void AllocateSourceArrays(const DecoratorImplT& impl,
                                             ArrayTupleType& arrays,
                                             svtkm::Id numValues,
                                             List<std::size_t, Indices...>)
  {
    CallAllocate(IsAllocatable{}, impl, numValues, svtkmstd::get<Indices>(arrays)...);
  }

  template <template <typename, std::size_t...> class List, std::size_t... Indices>
  SVTKM_CONT static void ShrinkSourceArrays(const DecoratorImplT& impl,
                                           ArrayTupleType& arrays,
                                           svtkm::Id numValues,
                                           List<std::size_t, Indices...>)
  {
    CallShrink(IsShrinkable{}, impl, numValues, svtkmstd::get<Indices>(arrays)...);
  }

#endif // SVTKM_USE_TAO_SEQ
};

} // end namespace decor

template <typename DecoratorImplT, typename... ArrayTs>
struct SVTKM_ALWAYS_EXPORT StorageTagDecorator
{
};

template <typename DecoratorImplT, typename... ArrayTs>
class Storage<typename decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>::ValueType,
              StorageTagDecorator<DecoratorImplT, ArrayTs...>>
{
  using Traits = decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>;
  using IndexList = typename Traits::IndexList;

public:
  using ArrayTupleType = typename Traits::ArrayTupleType;
  using ValueType = typename Traits::ValueType;
  using PortalType = typename Traits::PortalControlType;
  using PortalConstType = typename Traits::PortalConstControlType;

  SVTKM_CONT
  Storage()
    : Valid{ false }
  {
  }

  SVTKM_CONT
  Storage(const DecoratorImplT& impl, const ArrayTupleType& arrayTuple, svtkm::Id numValues)
    : Implementation(impl)
    , ArrayTuple{ arrayTuple }
    , NumberOfValues(numValues)
    , Valid{ true }
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return Traits::MakePortalControl(
      this->Implementation, this->ArrayTuple, this->NumberOfValues, IndexList{});
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return Traits::MakePortalConstControl(
      this->Implementation, this->ArrayTuple, this->NumberOfValues, IndexList{});
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->NumberOfValues;
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Valid);
    Traits::AllocateSourceArrays(this->Implementation, this->ArrayTuple, numValues, IndexList{});
    // If the above call doesn't throw, update our state.
    this->NumberOfValues = numValues;
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Valid);
    Traits::ShrinkSourceArrays(this->Implementation, this->ArrayTuple, numValues, IndexList{});
    // If the above call doesn't throw, update our state.
    this->NumberOfValues = numValues;
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Valid);
    // No-op. Again, could eventually be passed down to the implementation.
  }

  SVTKM_CONT
  const ArrayTupleType& GetArrayTuple() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->ArrayTuple;
  }

  SVTKM_CONT
  ArrayTupleType& GetArrayTuple()
  {
    SVTKM_ASSERT(this->Valid);
    return this->ArrayTuple;
  }

  SVTKM_CONT
  const DecoratorImplT& GetImplementation() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Implementation;
  }

  SVTKM_CONT
  DecoratorImplT& GetImplementation()
  {
    SVTKM_ASSERT(this->Valid);
    return this->Implementation;
  }

private:
  DecoratorImplT Implementation;
  ArrayTupleType ArrayTuple;
  svtkm::Id NumberOfValues;

  bool Valid;
};

template <typename DecoratorImplT, typename... ArrayTs>
struct DecoratorHandleTraits
{
  using StorageTraits = decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>;
  using ValueType = typename StorageTraits::ValueType;
  using StorageTag = StorageTagDecorator<DecoratorImplT, ArrayTs...>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;
  using Superclass = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

template <typename DecoratorImplT, typename... ArrayTs, typename Device>
class ArrayTransfer<typename decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>::ValueType,
                    StorageTagDecorator<DecoratorImplT, ArrayTs...>,
                    Device>
{
  SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

  using HandleTraits = DecoratorHandleTraits<DecoratorImplT, ArrayTs...>;
  using Traits = typename HandleTraits::StorageTraits;
  using IndexList = typename Traits::IndexList;
  using StorageType = typename HandleTraits::StorageType;

public:
  using ValueType = typename Traits::ValueType;

  using PortalControl = typename Traits::PortalControlType;
  using PortalConstControl = typename Traits::PortalConstControlType;

  using PortalExecution = typename Traits::template PortalExecutionType<Device>;
  using PortalConstExecution = typename Traits::template PortalConstExecutionType<Device>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData)) const
  {
    return Traits::MakePortalInput(this->Storage->GetImplementation(),
                                   this->Storage->GetArrayTuple(),
                                   this->Storage->GetNumberOfValues(),
                                   IndexList{},
                                   Device{});
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return Traits::MakePortalInPlace(this->Storage->GetImplementation(),
                                     this->Storage->GetArrayTuple(),
                                     this->Storage->GetNumberOfValues(),
                                     IndexList{},
                                     Device{});
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id)
  {
    return Traits::MakePortalOutput(this->Storage->GetImplementation(),
                                    this->Storage->GetArrayTuple(),
                                    this->Storage->GetNumberOfValues(),
                                    IndexList{},
                                    Device{});
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues) { this->Storage->Shrink(numValues); }


  SVTKM_CONT
  void ReleaseResources()
  {
    // no-op
  }

private:
  StorageType* Storage;
};

} // namespace internal

/// \brief A fancy ArrayHandle that can be used to modify the results from one
/// or more source ArrayHandle.
///
/// ArrayHandleDecorator is given a `DecoratorImplT` class and a list of one or
/// more source ArrayHandles. There are no restrictions on the size or type of
/// the source ArrayHandles.
///
/// The decorator implementation class is described below:
///
/// ```
/// struct ExampleDecoratorImplementation
/// {
///
///   // Takes one portal for each source array handle (only two shown).
///   // Returns a functor that defines:
///   //
///   // SVTKM_EXEC_CONT ValueType operator()(svtkm::Id id) const;
///   //
///   // which takes an index and returns a value which should be produced by
///   // the source arrays somehow. This ValueType will be the ValueType of the
///   // ArrayHandleDecorator.
///   //
///   // Both SomeFunctor::operator() and CreateFunctor must be const.
///   //
///   template <typename Portal1Type, typename Portal2Type>
///   SVTKM_CONT
///   SomeFunctor CreateFunctor(Portal1Type portal1, Portal2Type portal2) const;
///
///   // Takes one portal for each source array handle (only two shown).
///   // Returns a functor that defines:
///   //
///   // SVTKM_EXEC_CONT void operator()(svtkm::Id id, ValueType val) const;
///   //
///   // which takes an index and a value, which should be used to modify one
///   // or more of the source arrays.
///   //
///   // CreateInverseFunctor is optional; if not provided, the
///   // ArrayHandleDecorator will be read-only. In addition, if all of the
///   // source ArrayHandles are read-only, the inverse functor will not be used
///   // and the ArrayHandleDecorator will be read only.
///   //
///   // Both SomeInverseFunctor::operator() and CreateInverseFunctor must be
///   // const.
///   //
///   template <typename Portal1Type, typename Portal2Type>
///   SVTKM_CONT
///   SomeInverseFunctor CreateInverseFunctor(Portal1Type portal1,
///                                           Portal2Type portal2) const;
///
///   // Given a set of ArrayHandles and a size, implement what should happen
///   // to the source ArrayHandles when Allocate() is called on the decorator
///   // handle.
///   //
///   // AllocateSourceArrays is optional; if not provided, the
///   // ArrayHandleDecorator will throw if its Allocate method is called. If
///   // an implementation is present and doesn't throw, the
///   // ArrayHandleDecorator's internal state is updated to show `size` as the
///   // number of values.
///   template <typename Array1Type, typename Array2Type>
///   SVTKM_CONT
///   void AllocateSourceArrays(svtkm::Id size, Array1Type array1, Array2Type array2) const;
///
///   // Given a set of ArrayHandles and a size, implement what should happen to
///   // the source ArrayHandles when Shrink() is called on the decorator handle.
///   //
///   // ShrinkSourceArrays is optional; if not provided, the
///   // ArrayHandleDecorator will throw if its Shrink method is called. If
///   // an implementation is present and doesn't throw, the
///   // ArrayHandleDecorator's internal state is updated to show `size` as the
///   // number of values.
///   template <typename Array1Type, typename Array2Type>
///   SVTKM_CONT
///   void ShrinkSourceArrays(svtkm::Id size, Array1Type array1, Array2Type array2) const;
///
/// };
/// ```
///
/// There are several example DecoratorImpl classes provided in the
/// UnitTestArrayHandleDecorator test file.
///
template <typename DecoratorImplT, typename... ArrayTs>
class ArrayHandleDecorator
  : public internal::DecoratorHandleTraits<typename std::decay<DecoratorImplT>::type,
                                           typename std::decay<ArrayTs>::type...>::Superclass
{
private:
  using Traits = internal::DecoratorHandleTraits<typename std::decay<DecoratorImplT>::type,
                                                 typename std::decay<ArrayTs>::type...>;
  using StorageType = typename Traits::StorageType;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleDecorator,
                             (ArrayHandleDecorator<typename std::decay<DecoratorImplT>::type,
                                                   typename std::decay<ArrayTs>::type...>),
                             (typename Traits::Superclass));

  SVTKM_CONT
  ArrayHandleDecorator(svtkm::Id numValues,
                       const typename std::decay<DecoratorImplT>::type& impl,
                       const typename std::decay<ArrayTs>::type&... arrays)
    : Superclass{ StorageType{ impl, svtkmstd::make_tuple(arrays...), numValues } }
  {
  }
};

/// Create an ArrayHandleDecorator with the specified number of values that
/// uses the provided DecoratorImplT and source ArrayHandles.
///
template <typename DecoratorImplT, typename... ArrayTs>
SVTKM_CONT ArrayHandleDecorator<typename std::decay<DecoratorImplT>::type,
                               typename std::decay<ArrayTs>::type...>
make_ArrayHandleDecorator(svtkm::Id numValues, DecoratorImplT&& f, ArrayTs&&... arrays)
{
  using AHList = brigand::list<typename std::decay<ArrayTs>::type...>;
  SVTKM_STATIC_ASSERT_MSG(sizeof...(ArrayTs) > 0,
                         "Must specify at least one source array handle for "
                         "ArrayHandleDecorator. Consider using "
                         "ArrayHandleImplicit instead.");
  SVTKM_STATIC_ASSERT_MSG(internal::decor::AllAreArrayHandles<AHList>::value,
                         "Trailing template parameters for "
                         "ArrayHandleDecorator must be a list of ArrayHandle "
                         "types.");

  return { numValues, std::forward<DecoratorImplT>(f), std::forward<ArrayTs>(arrays)... };
}
}
} // namespace svtkm::cont

#ifdef SVTKM_USE_TAO_SEQ
#undef SVTKM_USE_TAO_SEQ
#endif

#endif //svtk_m_ArrayHandleDecorator_h
