//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleImplicit_h
#define svtk_m_cont_ArrayHandleImplicit_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageImplicit.h>

namespace svtkm
{
namespace cont
{

namespace detail
{

template <class FunctorType_>
class SVTKM_ALWAYS_EXPORT ArrayPortalImplicit;

/// A convenience class that provides a typedef to the appropriate tag for
/// a implicit array container.
template <typename FunctorType>
struct ArrayHandleImplicitTraits
{
  using ValueType = decltype(FunctorType{}(svtkm::Id{}));
  using StorageTag = svtkm::cont::StorageTagImplicit<ArrayPortalImplicit<FunctorType>>;
  using Superclass = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

/// \brief An array portal that returns the result of a functor
///
/// This array portal is similar to an implicit array i.e an array that is
/// defined functionally rather than actually stored in memory. The array
/// comprises a functor that is called for each index.
///
/// The \c ArrayPortalImplicit is used in an ArrayHandle with an
/// \c StorageImplicit container.
///
template <class FunctorType_>
class SVTKM_ALWAYS_EXPORT ArrayPortalImplicit
{
public:
  using ValueType = typename ArrayHandleImplicitTraits<FunctorType_>::ValueType;
  using FunctorType = FunctorType_;

  SVTKM_EXEC_CONT
  ArrayPortalImplicit()
    : Functor()
    , NumberOfValues(0)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalImplicit(FunctorType f, svtkm::Id numValues)
    : Functor(f)
    , NumberOfValues(numValues)
  {
  }

  SVTKM_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return this->Functor(index); }

private:
  FunctorType Functor;
  svtkm::Id NumberOfValues;
};

} // namespace detail

/// \brief An \c ArrayHandle that computes values on the fly.
///
/// \c ArrayHandleImplicit is a specialization of ArrayHandle.
/// It takes a user defined functor which is called with a given index value.
/// The functor returns the result of the functor as the value of this
/// array at that position.
///
template <class FunctorType>
class SVTKM_ALWAYS_EXPORT ArrayHandleImplicit
  : public detail::ArrayHandleImplicitTraits<FunctorType>::Superclass
{
private:
  using ArrayTraits = typename detail::ArrayHandleImplicitTraits<FunctorType>;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleImplicit,
                             (ArrayHandleImplicit<FunctorType>),
                             (typename ArrayTraits::Superclass));

  SVTKM_CONT
  ArrayHandleImplicit(FunctorType functor, svtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(functor, length))
  {
  }
};

/// make_ArrayHandleImplicit is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// arry.

template <typename FunctorType>
SVTKM_CONT svtkm::cont::ArrayHandleImplicit<FunctorType> make_ArrayHandleImplicit(FunctorType functor,
                                                                                svtkm::Id length)
{
  return ArrayHandleImplicit<FunctorType>(functor, length);
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

template <typename Functor>
struct SerializableTypeString<svtkm::cont::ArrayHandleImplicit<Functor>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Implicit<" + SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename Functor>
struct SerializableTypeString<svtkm::cont::ArrayHandle<
  typename svtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  svtkm::cont::StorageTagImplicit<svtkm::cont::detail::ArrayPortalImplicit<Functor>>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleImplicit<Functor>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename Functor>
struct Serialization<svtkm::cont::ArrayHandleImplicit<Functor>>
{
private:
  using Type = svtkm::cont::ArrayHandleImplicit<Functor>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetNumberOfValues());
    svtkmdiy::save(bb, obj.GetPortalConstControl().GetFunctor());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::Id count = 0;
    svtkmdiy::load(bb, count);

    Functor functor;
    svtkmdiy::load(bb, functor);

    obj = svtkm::cont::make_ArrayHandleImplicit(functor, count);
  }
};

template <typename Functor>
struct Serialization<svtkm::cont::ArrayHandle<
  typename svtkm::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  svtkm::cont::StorageTagImplicit<svtkm::cont::detail::ArrayPortalImplicit<Functor>>>>
  : Serialization<svtkm::cont::ArrayHandleImplicit<Functor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleImplicit_h
