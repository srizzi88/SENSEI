//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VecFromPortal_h
#define svtk_m_VecFromPortal_h

#include <svtkm/Math.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/internal/ArrayPortalValueReference.h>

namespace svtkm
{

/// \brief A short variable-length array from a window in an ArrayPortal.
///
/// The \c VecFromPortal class is a Vec-like class that holds an array portal
/// and exposes a small window of that portal as if it were a \c Vec.
///
template <typename PortalType>
class VecFromPortal
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortal()
    : NumComponents(0)
    , Offset(0)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortal(const PortalType& portal, svtkm::IdComponent numComponents = 0, svtkm::Id offset = 0)
    : Portal(portal)
    , NumComponents(numComponents)
    , Offset(offset)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <typename T, svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<T, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->NumComponents);
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Portal.Get(index + this->Offset);
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::internal::ArrayPortalValueReference<PortalType> operator[](svtkm::IdComponent index) const
  {
    return svtkm::internal::ArrayPortalValueReference<PortalType>(this->Portal,
                                                                 index + this->Offset);
  }

private:
  PortalType Portal;
  svtkm::IdComponent NumComponents;
  svtkm::Id Offset;
};

template <typename PortalType>
struct TypeTraits<svtkm::VecFromPortal<PortalType>>
{
private:
  using ComponentType = typename PortalType::ValueType;

public:
  using NumericTag = typename svtkm::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  static svtkm::VecFromPortal<PortalType> ZeroInitialization()
  {
    return svtkm::VecFromPortal<PortalType>();
  }
};

template <typename PortalType>
struct VecTraits<svtkm::VecFromPortal<PortalType>>
{
  using VecType = svtkm::VecFromPortal<PortalType>;

  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = svtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = svtkm::VecTraitsTagSizeVariable;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, svtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace svtkm

#endif //svtk_m_VecFromPortal_h
