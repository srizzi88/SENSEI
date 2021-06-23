//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DynamicCellSet_h
#define svtk_m_cont_DynamicCellSet_h

#include <svtkm/cont/CastAndCall.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetList.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Logging.h>

namespace svtkm
{
namespace cont
{

/// \brief Holds a cell set without having to specify concrete type.
///
/// \c DynamicCellSet holds a \c CellSet object using runtime polymorphism to
/// manage different subclass types and template parameters of the subclasses
/// rather than compile-time templates. This adds a programming convenience
/// that helps avoid a proliferation of templates. It also provides the
/// management necessary to interface SVTK-m with data sources where types will
/// not be known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in SVTK-m, \c DynamicCellSet contains a method named \c CastAndCall that
/// will determine the correct type from some known list of cell set types.
/// This mechanism is used internally by SVTK-m's worklet invocation mechanism
/// to determine the type when running algorithms.
///
/// By default, \c DynamicCellSet will assume that the value type in the array
/// matches one of the types specified by \c SVTKM_DEFAULT_CELL_SET_LIST.
/// This list can be changed by using the \c ResetCellSetList method. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple \c Dynamic* objects.
///
/// The actual implementation of \c DynamicCellSet is in a templated class
/// named \c DynamicCellSetBase, which is templated on the list of cell set
/// types. \c DynamicCellSet is really just a typedef of \c DynamicCellSetBase
/// with the default cell set list.
///
template <typename CellSetList>
class SVTKM_ALWAYS_EXPORT DynamicCellSetBase
{
  SVTKM_IS_LIST(CellSetList);

public:
  SVTKM_CONT
  DynamicCellSetBase() = default;

  template <typename CellSetType>
  SVTKM_CONT DynamicCellSetBase(const CellSetType& cellSet)
    : CellSet(std::make_shared<CellSetType>(cellSet))
  {
    SVTKM_IS_CELL_SET(CellSetType);
  }

  template <typename OtherCellSetList>
  SVTKM_CONT explicit DynamicCellSetBase(const DynamicCellSetBase<OtherCellSetList>& src)
    : CellSet(src.CellSet)
  {
  }

  SVTKM_CONT explicit DynamicCellSetBase(const std::shared_ptr<svtkm::cont::CellSet>& cs)
    : CellSet(cs)
  {
  }

  /// Returns true if this cell set is of the provided type.
  ///
  template <typename CellSetType>
  SVTKM_CONT bool IsType() const
  {
    return (dynamic_cast<CellSetType*>(this->CellSet.get()) != nullptr);
  }

  /// Returns true if this cell set is the same (or equivalent) type as the
  /// object provided.
  ///
  template <typename CellSetType>
  SVTKM_CONT bool IsSameType(const CellSetType&) const
  {
    return this->IsType<CellSetType>();
  }

  /// Returns this cell set cast to the given \c CellSet type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType to check if
  /// the cast can happen.
  ///
  template <typename CellSetType>
  SVTKM_CONT CellSetType& Cast() const
  {
    auto cellSetPointer = dynamic_cast<CellSetType*>(this->CellSet.get());
    if (cellSetPointer == nullptr)
    {
      SVTKM_LOG_CAST_FAIL(*this, CellSetType);
      throw svtkm::cont::ErrorBadType("Bad cast of dynamic cell set.");
    }
    SVTKM_LOG_CAST_SUCC(*this, *cellSetPointer);
    return *cellSetPointer;
  }

  /// Given a reference to a concrete \c CellSet object, attempt to downcast
  /// the contain cell set to the provided type and copy into the given \c
  /// CellSet object. Throws \c ErrorBadType if the cast does not work.
  /// Use \c IsType to check if the cast can happen.
  ///
  /// Note that this is a shallow copy. Any data in associated arrays are not
  /// copied.
  ///
  template <typename CellSetType>
  SVTKM_CONT void CopyTo(CellSetType& cellSet) const
  {
    cellSet = this->Cast<CellSetType>();
  }

  /// Changes the cell set types to try casting to when resolving this dynamic
  /// cell set, which is specified with a list tag like those in
  /// CellSetList.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic cell setobject.
  /// This method is particularly useful to narrow down (or expand) the types
  /// when using a cell set of particular constraints.
  ///
  template <typename NewCellSetList>
  SVTKM_CONT DynamicCellSetBase<NewCellSetList> ResetCellSetList(
    NewCellSetList = NewCellSetList()) const
  {
    SVTKM_IS_LIST(NewCellSetList);
    return DynamicCellSetBase<NewCellSetList>(*this);
  }

  /// Attempts to cast the held cell set to a specific concrete type, then call
  /// the given functor with the cast cell set. The cell sets tried in the cast
  /// are those in the \c CellSetList template argument of the \c
  /// DynamicCellSetBase class (or \c SVTKM_DEFAULT_CELL_SET_LIST for \c
  /// DynamicCellSet). You can use \c ResetCellSetList to get different
  /// behavior from \c CastAndCall.
  ///
  template <typename Functor, typename... Args>
  SVTKM_CONT void CastAndCall(Functor&& f, Args&&...) const;

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell set that is the same type as this one and
  /// returns a new dynamic cell set for it. This method is convenient when
  /// creating output data sets that should be the same type as some input cell
  /// set.
  ///
  SVTKM_CONT
  DynamicCellSetBase<CellSetList> NewInstance() const
  {
    DynamicCellSetBase<CellSetList> newCellSet;
    if (this->CellSet)
    {
      newCellSet.CellSet = this->CellSet->NewInstance();
    }
    return newCellSet;
  }

  SVTKM_CONT
  svtkm::cont::CellSet* GetCellSetBase() { return this->CellSet.get(); }

  SVTKM_CONT
  const svtkm::cont::CellSet* GetCellSetBase() const { return this->CellSet.get(); }

  SVTKM_CONT
  svtkm::Id GetNumberOfCells() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfCells() : 0;
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfFaces() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfFaces() : 0;
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfEdges() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfEdges() : 0;
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfPoints() const
  {
    return this->CellSet ? this->CellSet->GetNumberOfPoints() : 0;
  }

  SVTKM_CONT
  void ReleaseResourcesExecution()
  {
    if (this->CellSet)
    {
      this->CellSet->ReleaseResourcesExecution();
    }
  }

  SVTKM_CONT
  void PrintSummary(std::ostream& stream) const
  {
    if (this->CellSet)
    {
      this->CellSet->PrintSummary(stream);
    }
    else
    {
      stream << " DynamicCellSet = nullptr" << std::endl;
    }
  }

private:
  std::shared_ptr<svtkm::cont::CellSet> CellSet;

  template <typename>
  friend class DynamicCellSetBase;
};

//=============================================================================
// Free function casting helpers

/// Returns true if \c dynamicCellSet matches the type of CellSetType.
///
template <typename CellSetType, typename Ts>
SVTKM_CONT inline bool IsType(const svtkm::cont::DynamicCellSetBase<Ts>& dynamicCellSet)
{
  return dynamicCellSet.template IsType<CellSetType>();
}

/// Returns \c dynamicCellSet cast to the given \c CellSet type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename CellSetType, typename Ts>
SVTKM_CONT inline CellSetType Cast(const svtkm::cont::DynamicCellSetBase<Ts>& dynamicCellSet)
{
  return dynamicCellSet.template Cast<CellSetType>();
}

namespace detail
{

struct DynamicCellSetTry
{
  DynamicCellSetTry(const svtkm::cont::CellSet* const cellSetBase)
    : CellSetBase(cellSetBase)
  {
  }

  template <typename CellSetType, typename Functor, typename... Args>
  void operator()(CellSetType, Functor&& f, bool& called, Args&&... args) const
  {
    if (!called)
    {
      auto* cellset = dynamic_cast<const CellSetType*>(this->CellSetBase);
      if (cellset)
      {
        SVTKM_LOG_CAST_SUCC(*this->CellSetBase, *cellset);
        f(*cellset, std::forward<Args>(args)...);
        called = true;
      }
    }
  }

  const svtkm::cont::CellSet* const CellSetBase;
};

} // namespace detail

template <typename CellSetList>
template <typename Functor, typename... Args>
SVTKM_CONT void DynamicCellSetBase<CellSetList>::CastAndCall(Functor&& f, Args&&... args) const
{
  bool called = false;
  detail::DynamicCellSetTry tryCellSet(this->CellSet.get());
  svtkm::ListForEach(
    tryCellSet, CellSetList{}, std::forward<Functor>(f), called, std::forward<Args>(args)...);
  if (!called)
  {
    SVTKM_LOG_CAST_FAIL(*this, CellSetList);
    throw svtkm::cont::ErrorBadValue("Could not find appropriate cast for cell set.");
  }
}

using DynamicCellSet = DynamicCellSetBase<SVTKM_DEFAULT_CELL_SET_LIST>;

namespace internal
{

template <typename CellSetList>
struct DynamicTransformTraits<svtkm::cont::DynamicCellSetBase<CellSetList>>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

namespace internal
{

/// Checks to see if the given object is a dynamic cell set. It contains a
/// typedef named \c type that is either std::true_type or std::false_type.
/// Both of these have a typedef named value with the respective boolean value.
///
template <typename T>
struct DynamicCellSetCheck
{
  using type = std::false_type;
};

template <typename CellSetList>
struct DynamicCellSetCheck<svtkm::cont::DynamicCellSetBase<CellSetList>>
{
  using type = std::true_type;
};

#define SVTKM_IS_DYNAMIC_CELL_SET(T)                                                                \
  SVTKM_STATIC_ASSERT(::svtkm::cont::internal::DynamicCellSetCheck<T>::type::value)

#define SVTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(T)                                                      \
  SVTKM_STATIC_ASSERT(::svtkm::cont::internal::CellSetCheck<T>::type::value ||                       \
                     ::svtkm::cont::internal::DynamicCellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

namespace internal
{

struct DynamicCellSetSerializeFunctor
{
  template <typename CellSetType>
  void operator()(const CellSetType& cs, BinaryBuffer& bb) const
  {
    svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<CellSetType>::Get());
    svtkmdiy::save(bb, cs);
  }
};

template <typename CellSetTypes>
struct DynamicCellSetDeserializeFunctor
{
  template <typename CellSetType>
  void operator()(CellSetType,
                  svtkm::cont::DynamicCellSetBase<CellSetTypes>& dh,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    if (!success && (typeString == svtkm::cont::SerializableTypeString<CellSetType>::Get()))
    {
      CellSetType cs;
      svtkmdiy::load(bb, cs);
      dh = svtkm::cont::DynamicCellSetBase<CellSetTypes>(cs);
      success = true;
    }
  }
};

} // internal

template <typename CellSetTypes>
struct Serialization<svtkm::cont::DynamicCellSetBase<CellSetTypes>>
{
private:
  using Type = svtkm::cont::DynamicCellSetBase<CellSetTypes>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::DynamicCellSetSerializeFunctor{}, bb);
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    svtkmdiy::load(bb, typeString);

    bool success = false;
    svtkm::ListForEach(internal::DynamicCellSetDeserializeFunctor<CellSetTypes>{},
                      CellSetTypes{},
                      obj,
                      typeString,
                      success,
                      bb);

    if (!success)
    {
      throw svtkm::cont::ErrorBadType("Error deserializing DynamicCellSet. Message TypeString: " +
                                     typeString);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_DynamicCellSet_h
