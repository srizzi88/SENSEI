//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_VariantArrayHandle_h
#define svtk_m_cont_VariantArrayHandle_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/TypeList.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/ArrayHandleMultiplexer.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/CastAndCall.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/StorageList.h>

#include <svtkm/cont/internal/VariantArrayHandleContainer.h>

namespace svtkm
{
namespace cont
{
/// \brief Holds an array handle without having to specify template parameters.
///
/// \c VariantArrayHandle holds an \c ArrayHandle or \c ArrayHandleVirtual
/// object using runtime polymorphism to manage different value types and
/// storage rather than compile-time templates. This adds a programming
/// convenience that helps avoid a proliferation of templates. It also provides
/// the management necessary to interface SVTK-m with data sources where types
/// will not be known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in SVTK-m, \c VariantArrayHandle contains a method named \c CastAndCall that
/// will determine the correct type from some known list of types. It returns
/// an ArrayHandleVirtual which type erases the storage type by using polymorphism.
/// This mechanism is used internally by SVTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// By default, \c VariantArrayHandle will assume that the value type in the
/// array matches one of the types specified by \c SVTKM_DEFAULT_TYPE_LIST
/// This list can be changed by using the \c ResetTypes. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple \c VariantArrayHandle
/// objects.
///
/// The actual implementation of \c VariantArrayHandle is in a templated class
/// named \c VariantArrayHandleBase, which is templated on the list of
/// component types.
///
template <typename TypeList>
class SVTKM_ALWAYS_EXPORT VariantArrayHandleBase
{
public:
  SVTKM_CONT
  VariantArrayHandleBase() = default;

  template <typename T, typename Storage>
  SVTKM_CONT VariantArrayHandleBase(const svtkm::cont::ArrayHandle<T, Storage>& array)
    : ArrayContainer(std::make_shared<internal::VariantArrayHandleContainer<T>>(
        svtkm::cont::ArrayHandleVirtual<T>{ array }))
  {
  }

  template <typename T>
  explicit SVTKM_CONT VariantArrayHandleBase(
    const svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>& array)
    : ArrayContainer(std::make_shared<internal::VariantArrayHandleContainer<T>>(array))
  {
  }

  template <typename OtherTypeList>
  SVTKM_CONT explicit VariantArrayHandleBase(const VariantArrayHandleBase<OtherTypeList>& src)
    : ArrayContainer(internal::variant::GetContainer::Extract(src))
  {
  }

  SVTKM_CONT VariantArrayHandleBase(const VariantArrayHandleBase&) = default;
  SVTKM_CONT VariantArrayHandleBase(VariantArrayHandleBase&&) noexcept = default;

  SVTKM_CONT
  ~VariantArrayHandleBase() {}

  SVTKM_CONT
  VariantArrayHandleBase<TypeList>& operator=(const VariantArrayHandleBase<TypeList>&) = default;

  SVTKM_CONT
  VariantArrayHandleBase<TypeList>& operator=(VariantArrayHandleBase<TypeList>&&) noexcept =
    default;


  /// Returns true if this array matches the array handle type passed in.
  ///
  template <typename ArrayHandleType>
  SVTKM_CONT bool IsType() const
  {
    return internal::variant::IsType<ArrayHandleType>(this->ArrayContainer.get());
  }

  /// Returns true if this array matches the ValueType type passed in.
  ///
  template <typename T>
  SVTKM_CONT bool IsValueType() const
  {
    return internal::variant::IsValueType<T>(this->ArrayContainer.get());
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  SVTKM_CONT ArrayHandleType Cast() const
  {
    return internal::variant::Cast<ArrayHandleType>(this->ArrayContainer.get());
  }

  /// Returns this array cast to a \c ArrayHandleVirtual of the given type.
  /// This will perform type conversions as necessary, and will log warnings
  /// if the conversion is lossy.
  ///
  /// This method internally uses CastAndCall. A custom storage tag list may
  /// be specified in the second template parameter, which will be passed to
  /// the CastAndCall.
  ///
  template <typename T, typename StorageTagList = SVTKM_DEFAULT_STORAGE_LIST>
  SVTKM_CONT svtkm::cont::ArrayHandleVirtual<T> AsVirtual() const
  {
    SVTKM_IS_LIST(StorageTagList);
    svtkm::cont::internal::variant::ForceCastToVirtual caster;
    svtkm::cont::ArrayHandleVirtual<T> output;
    this->CastAndCall(StorageTagList{}, caster, output);
    return output;
  }

  /// Returns this array cast to a \c ArrayHandleMultiplexer of the given type.
  /// This will attempt to cast the internal array to each supported type of
  /// the multiplexer. If none are supported, an invalid ArrayHandleMultiplexer
  /// is returned.
  ///
  /// As a special case, if one of the arrays in the \c ArrayHandleMultiplexer's
  /// type list is an \c ArrayHandleCast, then the multiplexer will look for type
  /// type of array being cast rather than an actual cast array.
  ///
  ///@{
  template <typename... T>
  SVTKM_CONT void AsMultiplexer(svtkm::cont::ArrayHandleMultiplexer<T...>& result) const;

  template <typename ArrayHandleMultiplexerType>
  SVTKM_CONT ArrayHandleMultiplexerType AsMultiplexer() const
  {
    ArrayHandleMultiplexerType result;
    this->AsMultiplexer(result);
    return result;
  }
  ///@}

  /// Given a references to an ArrayHandle object, casts this array to the
  /// ArrayHandle's type and sets the given ArrayHandle to this array. Throws
  /// \c ErrorBadType if the cast does not work. Use \c
  /// ArrayHandleType to check if the cast can happen.
  ///
  /// Note that this is a shallow copy. The data are not copied and a change
  /// in the data in one array will be reflected in the other.
  ///
  template <typename ArrayHandleType>
  SVTKM_CONT void CopyTo(ArrayHandleType& array) const
  {
    SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    array = this->Cast<ArrayHandleType>();
  }

  /// Changes the types to try casting to when resolving this variant array,
  /// which is specified with a list tag like those in TypeList.h. Since C++
  /// does not allow you to actually change the template arguments, this method
  /// returns a new variant array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template <typename NewTypeList>
  SVTKM_CONT VariantArrayHandleBase<NewTypeList> ResetTypes(NewTypeList = NewTypeList()) const
  {
    SVTKM_IS_LIST(NewTypeList);
    return VariantArrayHandleBase<NewTypeList>(*this);
  }

  //@{
  /// \brief Call a functor using the underlying array type.
  ///
  /// \c CastAndCall Attempts to cast the held array to a specific value type,
  /// then call the given functor with the cast array. The types
  /// tried in the cast are those in the lists defined by the TypeList.
  /// By default \c VariantArrayHandle set this to \c SVTKM_DEFAULT_TYPE_LIST.
  ///
  /// In addition to the value type, an \c ArrayHandle also requires a storage tag.
  /// By default, \c CastAndCall attempts to cast the array using the storage tags
  /// listed in \c SVTKM_DEFAULT_STORAGE_LIST. You can optionally give a custom
  /// list of storage tags as the second argument. If the storage of the underlying
  /// array does not match any of the storage tags given, then the array will
  /// be cast to an \c ArrayHandleVirtual, which can hold any array given the
  /// appropriate value type. To always use \c ArrayHandleVirtual, pass
  /// \c svtkm::ListEmpty as thefirst argument.
  ///
  /// As previous stated, if a storage tag list is provided, it is given in the
  /// first argument. The functor to call with the cast array is given as the next
  /// argument (or the first argument if a storage tag list is not provided).
  /// The remaning arguments, if any, are passed to the functor.
  ///
  /// The functor will be called with the cast array as its first argument. Any
  /// remaining arguments are passed from the arguments to \c CastAndCall.
  ///
  template <typename FunctorOrStorageList, typename... Args>
  SVTKM_CONT void CastAndCall(FunctorOrStorageList&& functorOrStorageList, Args&&... args) const
  {
    this->CastAndCallImpl(svtkm::internal::IsList<FunctorOrStorageList>(),
                          std::forward<FunctorOrStorageList>(functorOrStorageList),
                          std::forward<Args>(args)...);
  }

  template <typename Functor>
  SVTKM_CONT void CastAndCall(Functor&& f) const
  {
    this->CastAndCallImpl(std::false_type(), std::forward<Functor>(f));
  }
  //@}

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new variant array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  SVTKM_CONT
  VariantArrayHandleBase<TypeList> NewInstance() const
  {
    VariantArrayHandleBase<TypeList> instance;
    instance.ArrayContainer = this->ArrayContainer->NewInstance();
    return instance;
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  void ReleaseResourcesExecution() { return this->ArrayContainer->ReleaseResourcesExecution(); }


  /// Releases all resources in both the control and execution environments.
  ///
  void ReleaseResources() { return this->ArrayContainer->ReleaseResources(); }

  /// \brief Get the number of components in each array value.
  ///
  /// This method will query the array type for the number of components in
  /// each value of the array. The number of components is determined by
  /// the \c VecTraits::NUM_COMPONENTS trait class.
  ///
  SVTKM_CONT
  svtkm::IdComponent GetNumberOfComponents() const
  {
    return this->ArrayContainer->GetNumberOfComponents();
  }

  /// \brief Get the number of values in the array.
  ///
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->ArrayContainer->GetNumberOfValues(); }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const { this->ArrayContainer->PrintSummary(out); }

private:
  friend struct internal::variant::GetContainer;
  std::shared_ptr<svtkm::cont::internal::VariantArrayHandleContainerBase> ArrayContainer;

  template <typename Functor, typename... Args>
  SVTKM_CONT void CastAndCallImpl(std::false_type, Functor&& f, Args&&... args) const
  {
    this->CastAndCallImpl(std::true_type(),
                          SVTKM_DEFAULT_STORAGE_LIST(),
                          std::forward<Functor>(f),
                          std::forward<Args>(args)...);
  }

  template <typename StorageTagList, typename Functor, typename... Args>
  SVTKM_CONT void CastAndCallImpl(std::true_type, StorageTagList, Functor&& f, Args&&...) const;
};

using VariantArrayHandle = svtkm::cont::VariantArrayHandleBase<SVTKM_DEFAULT_TYPE_LIST>;


//=============================================================================
// Free function casting helpers

/// Returns true if \c variant matches the type of ArrayHandleType.
///
template <typename ArrayHandleType, typename Ts>
SVTKM_CONT inline bool IsType(const svtkm::cont::VariantArrayHandleBase<Ts>& variant)
{
  return variant.template IsType<ArrayHandleType>();
}

/// Returns \c variant cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType, typename Ts>
SVTKM_CONT inline ArrayHandleType Cast(const svtkm::cont::VariantArrayHandleBase<Ts>& variant)
{
  return variant.template Cast<ArrayHandleType>();
}

//=============================================================================
// Out of class implementations

namespace detail
{

struct VariantArrayHandleTry
{
  template <typename T, typename Storage, typename Functor, typename... Args>
  void operator()(svtkm::List<T, Storage>,
                  Functor&& f,
                  bool& called,
                  const svtkm::cont::internal::VariantArrayHandleContainerBase& container,
                  Args&&... args) const
  {
    using DerivedArrayType = svtkm::cont::ArrayHandle<T, Storage>;
    if (!called && svtkm::cont::internal::variant::IsType<DerivedArrayType>(&container))
    {
      called = true;
      const auto* derivedContainer =
        static_cast<const svtkm::cont::internal::VariantArrayHandleContainer<T>*>(&container);
      DerivedArrayType derivedArray = derivedContainer->Array.template Cast<DerivedArrayType>();
      SVTKM_LOG_CAST_SUCC(container, derivedArray);

      // If you get a compile error here, it means that you have called CastAndCall for a
      // svtkm::cont::VariantArrayHandle and the arguments of the functor do not match those
      // being passed. This is often because it is calling the functor with an ArrayHandle
      // type that was not expected. Either add overloads to the functor to accept all
      // possible array types or constrain the types tried for the CastAndCall. Note that
      // the functor will be called with an array of type svtkm::cont::ArrayHandle<T, S>.
      // Directly using a subclass of ArrayHandle (e.g. svtkm::cont::ArrayHandleConstant<T>)
      // might not work.
      f(derivedArray, std::forward<Args>(args)...);
    }
  }
};

struct VariantArrayHandleTryFallback
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T,
                  Functor&& f,
                  bool& called,
                  const svtkm::cont::internal::VariantArrayHandleContainerBase& container,
                  Args&&... args) const
  {
    if (!called && svtkm::cont::internal::variant::IsValueType<T>(&container))
    {
      called = true;
      const auto* derived =
        static_cast<const svtkm::cont::internal::VariantArrayHandleContainer<T>*>(&container);
      SVTKM_LOG_CAST_SUCC(container, derived);

      // If you get a compile error here, it means that you have called CastAndCall for a
      // svtkm::cont::VariantArrayHandle and the arguments of the functor do not match those
      // being passed. This is often because it is calling the functor with an ArrayHandle
      // type that was not expected. Either add overloads to the functor to accept all
      // possible array types or constrain the types tried for the CastAndCall. Note that
      // the functor will be called with an array of type svtkm::cont::ArrayHandle<T, S>.
      // Directly using a subclass of ArrayHandle (e.g. svtkm::cont::ArrayHandleConstant<T>)
      // might not work.
      f(derived->Array, std::forward<Args>(args)...);
    }
  }
};

template <typename T>
struct IsUndefinedStorage
{
};
template <typename T, typename U>
struct IsUndefinedStorage<svtkm::List<T, U>> : svtkm::cont::internal::IsInValidArrayHandle<T, U>
{
};

template <typename TypeList, typename StorageList>
using ListDynamicTypes =
  svtkm::ListRemoveIf<svtkm::ListCross<TypeList, StorageList>, IsUndefinedStorage>;


SVTKM_CONT_EXPORT void ThrowCastAndCallException(
  const svtkm::cont::internal::VariantArrayHandleContainerBase&,
  const std::type_info&);

} // namespace detail



template <typename TypeList>
template <typename StorageTagList, typename Functor, typename... Args>
SVTKM_CONT void VariantArrayHandleBase<TypeList>::CastAndCallImpl(std::true_type,
                                                                 StorageTagList,
                                                                 Functor&& f,
                                                                 Args&&... args) const
{
  using crossProduct = detail::ListDynamicTypes<TypeList, StorageTagList>;

  bool called = false;
  const auto& ref = *this->ArrayContainer;
  svtkm::ListForEach(detail::VariantArrayHandleTry{},
                    crossProduct{},
                    std::forward<Functor>(f),
                    called,
                    ref,
                    std::forward<Args>(args)...);
  if (!called)
  {
    // try to fall back to using ArrayHandleVirtual
    svtkm::ListForEach(detail::VariantArrayHandleTryFallback{},
                      TypeList{},
                      std::forward<Functor>(f),
                      called,
                      ref,
                      std::forward<Args>(args)...);
  }
  if (!called)
  {
    // throw an exception
    SVTKM_LOG_CAST_FAIL(*this, TypeList);
    detail::ThrowCastAndCallException(ref, typeid(TypeList));
  }
}

namespace detail
{

struct VariantArrayHandleTryMultiplexer
{
  template <typename T, typename Storage, typename... TypeList, typename... ArrayTypes>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<T, Storage>&,
                            const svtkm::cont::VariantArrayHandleBase<TypeList...>& self,
                            svtkm::cont::ArrayHandleMultiplexer<ArrayTypes...>& result) const
  {
    svtkm::cont::ArrayHandle<T, Storage> targetArray;
    bool foundArray = false;
    this->FetchArray(targetArray, self, foundArray, result.IsValid());
    if (foundArray)
    {
      result.SetArray(targetArray);
    }
  }

private:
  template <typename T, typename Storage, typename... TypeList>
  SVTKM_CONT void FetchArrayExact(svtkm::cont::ArrayHandle<T, Storage>& targetArray,
                                 const svtkm::cont::VariantArrayHandleBase<TypeList...>& self,
                                 bool& foundArray) const
  {
    using ArrayType = svtkm::cont::ArrayHandle<T, Storage>;
    if (self.template IsType<ArrayType>())
    {
      targetArray = self.template Cast<ArrayType>();
      foundArray = true;
    }
    else
    {
      foundArray = false;
    }
  }

  template <typename T, typename Storage, typename... TypeList>
  SVTKM_CONT void FetchArray(svtkm::cont::ArrayHandle<T, Storage>& targetArray,
                            const svtkm::cont::VariantArrayHandleBase<TypeList...>& self,
                            bool& foundArray,
                            bool svtkmNotUsed(foundArrayInPreviousCall)) const
  {
    this->FetchArrayExact(targetArray, self, foundArray);
  }

  // Special condition for transformed arrays. Instead of pulling out the
  // transform, pull out the array that is being transformed.
  template <typename T,
            typename SrcArray,
            typename ForwardTransform,
            typename ReverseTransform,
            typename... TypeList>
  SVTKM_CONT void FetchArray(
    svtkm::cont::ArrayHandle<
      T,
      svtkm::cont::internal::StorageTagTransform<SrcArray, ForwardTransform, ReverseTransform>>&
      targetArray,
    const svtkm::cont::VariantArrayHandleBase<TypeList...>& self,
    bool& foundArray,
    bool foundArrayInPreviousCall) const
  {
    // Attempt to get the array itself first
    this->FetchArrayExact(targetArray, self, foundArray);

    // Try to get the array to be transformed first, but only do so if the array was not already
    // found in another call to this functor. This is to give precedence to getting the array
    // exactly rather than creating our own transform.
    if (!foundArray && !foundArrayInPreviousCall)
    {
      SrcArray srcArray;
      this->FetchArray(srcArray, self, foundArray, foundArrayInPreviousCall);
      if (foundArray)
      {
        targetArray =
          svtkm::cont::ArrayHandleTransform<SrcArray, ForwardTransform, ReverseTransform>(srcArray);
      }
    }
  }

  // Special condition for cast arrays. Instead of pulling out an ArrayHandleCast, pull out
  // the array that is being cast.
  template <typename TargetT, typename SourceT, typename SourceStorage, typename... TypeList>
  SVTKM_CONT void FetchArray(
    svtkm::cont::ArrayHandle<TargetT, svtkm::cont::StorageTagCast<SourceT, SourceStorage>>&
      targetArray,
    const svtkm::cont::VariantArrayHandleBase<TypeList...>& self,
    bool& foundArray,
    bool foundArrayInPreviousCall) const
  {
    // Attempt to get the array itself first
    this->FetchArrayExact(targetArray, self, foundArray);

    // Try to get the array to be transformed first, but only do so if the array was not already
    // found in another call to this functor. This is to give precedence to getting the array
    // exactly rather than creating our own transform.
    if (!foundArray && !foundArrayInPreviousCall)
    {
      using SrcArray = svtkm::cont::ArrayHandle<SourceT, SourceStorage>;
      SrcArray srcArray;
      this->FetchArray(srcArray, self, foundArray, foundArrayInPreviousCall);
      if (foundArray)
      {
        targetArray =
          svtkm::cont::ArrayHandleCast<TargetT, svtkm::cont::ArrayHandle<SourceT, SourceStorage>>(
            srcArray);
      }
    }
  }
};

} // namespace detail

template <typename TypeList>
template <typename... T>
inline SVTKM_CONT void VariantArrayHandleBase<TypeList>::AsMultiplexer(
  svtkm::cont::ArrayHandleMultiplexer<T...>& result) const
{
  // Make sure IsValid is clear
  result = svtkm::cont::ArrayHandleMultiplexer<T...>{};
  svtkm::ListForEach(detail::VariantArrayHandleTryMultiplexer{}, svtkm::List<T...>{}, *this, result);
}

namespace internal
{

template <typename TypeList>
struct DynamicTransformTraits<svtkm::cont::VariantArrayHandleBase<TypeList>>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace svtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

namespace internal
{

struct VariantArrayHandleSerializeFunctor
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& ah, BinaryBuffer& bb) const
  {
    svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<ArrayHandleType>::Get());
    svtkmdiy::save(bb, ah);
  }
};

struct VariantArrayHandleDeserializeFunctor
{
  template <typename T, typename TypeList>
  void operator()(T,
                  svtkm::cont::VariantArrayHandleBase<TypeList>& dh,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    using ArrayHandleType = svtkm::cont::ArrayHandleVirtual<T>;

    if (!success && (typeString == svtkm::cont::SerializableTypeString<ArrayHandleType>::Get()))
    {
      ArrayHandleType ah;
      svtkmdiy::load(bb, ah);
      dh = svtkm::cont::VariantArrayHandleBase<TypeList>(ah);
      success = true;
    }
  }
};

} // internal

template <typename TypeList>
struct Serialization<svtkm::cont::VariantArrayHandleBase<TypeList>>
{
private:
  using Type = svtkm::cont::VariantArrayHandleBase<TypeList>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(svtkm::ListEmpty(), internal::VariantArrayHandleSerializeFunctor{}, bb);
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    svtkmdiy::load(bb, typeString);

    bool success = false;
    svtkm::ListForEach(
      internal::VariantArrayHandleDeserializeFunctor{}, TypeList{}, obj, typeString, success, bb);

    if (!success)
    {
      throw svtkm::cont::ErrorBadType(
        "Error deserializing VariantArrayHandle. Message TypeString: " + typeString);
    }
  }
};

} // diy
/// @endcond SERIALIZATION


#endif //svtk_m_virts_VariantArrayHandle_h
