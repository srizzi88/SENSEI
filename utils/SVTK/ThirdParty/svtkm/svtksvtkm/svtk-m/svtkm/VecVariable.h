//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VecVariable_h
#define svtk_m_VecVariable_h

#include <svtkm/Assert.h>
#include <svtkm/Math.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{

/// \brief A short variable-length array with maximum length.
///
/// The \c VecVariable class is a Vec-like class that holds a short array of
/// some maximum length. To avoid dynamic allocations, the maximum length is
/// specified at compile time. Internally, \c VecVariable holds a \c Vec of
/// the maximum length and exposes a subsection of it.
///
template <typename T, svtkm::IdComponent MaxSize>
class VecVariable
{
public:
  using ComponentType = T;

  SVTKM_EXEC_CONT
  VecVariable()
    : NumComponents(0)
  {
  }

  template <typename SrcVecType>
  SVTKM_EXEC_CONT VecVariable(const SrcVecType& src)
    : NumComponents(src.GetNumberOfComponents())
  {
    SVTKM_ASSERT(this->NumComponents <= MaxSize);
    for (svtkm::IdComponent index = 0; index < this->NumComponents; index++)
    {
      this->Data[index] = src[index];
    }
  }

  SVTKM_EXEC_CONT
  inline svtkm::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT inline void CopyInto(svtkm::Vec<ComponentType, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->NumComponents);
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Data[index];
    }
  }

  SVTKM_EXEC_CONT
  inline const ComponentType& operator[](svtkm::IdComponent index) const
  {
    return this->Data[index];
  }

  SVTKM_EXEC_CONT
  inline ComponentType& operator[](svtkm::IdComponent index) { return this->Data[index]; }

  SVTKM_EXEC_CONT
  void Append(ComponentType value)
  {
    SVTKM_ASSERT(this->NumComponents < MaxSize);
    this->Data[this->NumComponents] = value;
    this->NumComponents++;
  }

private:
  svtkm::Vec<T, MaxSize> Data;
  svtkm::IdComponent NumComponents;
};

template <typename T, svtkm::IdComponent MaxSize>
struct TypeTraits<svtkm::VecVariable<T, MaxSize>>
{
  using NumericTag = typename svtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_EXEC_CONT
  static svtkm::VecVariable<T, MaxSize> ZeroInitialization()
  {
    return svtkm::VecVariable<T, MaxSize>();
  }
};

template <typename T, svtkm::IdComponent MaxSize>
struct VecTraits<svtkm::VecVariable<T, MaxSize>>
{
  using VecType = svtkm::VecVariable<T, MaxSize>;

  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = svtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = svtkm::VecTraitsTagSizeVariable;

  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  SVTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, svtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }
  SVTKM_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, svtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  SVTKM_EXEC_CONT
  static void SetComponent(VecType& vector,
                           svtkm::IdComponent componentIndex,
                           const ComponentType& value)
  {
    vector[componentIndex] = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = svtkm::VecVariable<NewComponentType, MaxSize>;

  template <typename NewComponentType>
  using ReplaceBaseComponentType = svtkm::VecVariable<
    typename svtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>,
    MaxSize>;

  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace svtkm

#endif //svtk_m_VecVariable_h
