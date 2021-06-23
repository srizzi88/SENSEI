//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VecFromPortalPermute_h
#define svtk_m_VecFromPortalPermute_h

#include <svtkm/Math.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{

/// \brief A short vector from an ArrayPortal and a vector of indices.
///
/// The \c VecFromPortalPermute class is a Vec-like class that holds an array
/// portal and a second Vec-like containing indices into the array. Each value
/// of this vector is the value from the array with the respective index.
///
template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortalPermute() {}

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType& portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfComponents() const { return this->Indices->GetNumberOfComponents(); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<ComponentType, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->GetNumberOfComponents());
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ComponentType operator[](svtkm::IdComponent index) const
  {
    return this->Portal.Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  PortalType Portal;
};

template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute<IndexVecType, const PortalType*>
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortalPermute() {}

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType* const portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfComponents() const { return this->Indices->GetNumberOfComponents(); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<ComponentType, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->GetNumberOfComponents());
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ComponentType operator[](svtkm::IdComponent index) const
  {
    return this->Portal->Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  const PortalType* const Portal;
};

template <typename IndexVecType, typename PortalType>
struct TypeTraits<svtkm::VecFromPortalPermute<IndexVecType, PortalType>>
{
private:
  using VecType = svtkm::VecFromPortalPermute<IndexVecType, PortalType>;
  using ComponentType = typename PortalType::ValueType;

public:
  using NumericTag = typename svtkm::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  static VecType ZeroInitialization() { return VecType(); }
};

template <typename IndexVecType, typename PortalType>
struct VecTraits<svtkm::VecFromPortalPermute<IndexVecType, PortalType>>
{
  using VecType = svtkm::VecFromPortalPermute<IndexVecType, PortalType>;

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

template <typename IndexVecType, typename PortalType>
inline SVTKM_EXEC VecFromPortalPermute<IndexVecType, PortalType> make_VecFromPortalPermute(
  const IndexVecType* index,
  const PortalType& portal)
{
  return VecFromPortalPermute<IndexVecType, PortalType>(index, portal);
}

template <typename IndexVecType, typename PortalType>
inline SVTKM_EXEC VecFromPortalPermute<IndexVecType, const PortalType*> make_VecFromPortalPermute(
  const IndexVecType* index,
  const PortalType* const portal)
{
  return VecFromPortalPermute<IndexVecType, const PortalType*>(index, portal);
}

} // namespace svtkm

#endif //svtk_m_VecFromPortalPermute_h
