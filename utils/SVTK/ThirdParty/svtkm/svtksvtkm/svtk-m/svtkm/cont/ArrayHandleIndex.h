//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleIndex_h
#define svtk_m_cont_ArrayHandleIndex_h

#include <svtkm/cont/ArrayHandleImplicit.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagIndex
{
};

namespace internal
{

struct SVTKM_ALWAYS_EXPORT IndexFunctor
{
  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id index) const { return index; }
};

using StorageTagIndexSuperclass =
  typename svtkm::cont::ArrayHandleImplicit<IndexFunctor>::StorageTag;

template <>
struct Storage<svtkm::Id, svtkm::cont::StorageTagIndex> : Storage<svtkm::Id, StorageTagIndexSuperclass>
{
  using Superclass = Storage<svtkm::Id, StorageTagIndexSuperclass>;
  using Superclass::Superclass;
};

template <typename Device>
struct ArrayTransfer<svtkm::Id, svtkm::cont::StorageTagIndex, Device>
  : ArrayTransfer<svtkm::Id, StorageTagIndexSuperclass, Device>
{
  using Superclass = ArrayTransfer<svtkm::Id, StorageTagIndexSuperclass, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// \brief An implicit array handle containing the its own indices.
///
/// \c ArrayHandleIndex is an implicit array handle containing the values
/// 0, 1, 2, 3,... to a specified size. Every value in the array is the same
/// as the index to that value.
///
class ArrayHandleIndex : public svtkm::cont::ArrayHandle<svtkm::Id, StorageTagIndex>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleIndex,
                                (svtkm::cont::ArrayHandle<svtkm::Id, StorageTagIndex>));

  SVTKM_CONT
  ArrayHandleIndex(svtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(internal::IndexFunctor{}, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleIndex. It takes the
/// size of the array and generates an array holding svtkm::Id from [0, size - 1]
SVTKM_CONT inline svtkm::cont::ArrayHandleIndex make_ArrayHandleIndex(svtkm::Id length)
{
  return svtkm::cont::ArrayHandleIndex(length);
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

template <>
struct SerializableTypeString<svtkm::cont::ArrayHandleIndex>
{
  static SVTKM_CONT const std::string Get() { return "AH_Index"; }
};

template <>
struct SerializableTypeString<svtkm::cont::ArrayHandle<svtkm::Id, svtkm::cont::StorageTagIndex>>
  : SerializableTypeString<svtkm::cont::ArrayHandleIndex>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<svtkm::cont::ArrayHandleIndex>
{
private:
  using BaseType = svtkm::cont::ArrayHandle<svtkm::Id, svtkm::cont::StorageTagIndex>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetNumberOfValues());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::Id length = 0;
    svtkmdiy::load(bb, length);

    obj = svtkm::cont::ArrayHandleIndex(length);
  }
};

template <>
struct Serialization<svtkm::cont::ArrayHandle<svtkm::Id, svtkm::cont::StorageTagIndex>>
  : Serialization<svtkm::cont::ArrayHandleIndex>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleIndex_h
