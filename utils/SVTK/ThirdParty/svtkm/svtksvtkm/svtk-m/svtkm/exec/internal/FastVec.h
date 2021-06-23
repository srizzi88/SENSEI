//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_internal_FastVec_h
#define svtk_m_exec_internal_FastVec_h

#include <svtkm/Types.h>
#include <svtkm/VecVariable.h>

namespace svtkm
{
namespace exec
{
namespace internal
{

/// Use this class to convert Vecs of any type to an efficient stack based Vec
/// type. The template parameters are the input Vec type and the maximum
/// number of components it may have. Specializations exist to optimize
/// the copy and stack usage away for already efficient types.
/// This class is useful when several accesses will be performed on
/// potentially inefficient Vec types such as VecFromPortalPermute.
///
template <typename VecType, svtkm::IdComponent MaxSize>
class FastVec
{
public:
  using Type = svtkm::VecVariable<typename VecType::ComponentType, MaxSize>;

  explicit SVTKM_EXEC FastVec(const VecType& vec)
    : Vec(vec)
  {
  }

  SVTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  Type Vec;
};

template <typename ComponentType, svtkm::IdComponent NumComponents, svtkm::IdComponent MaxSize>
class FastVec<svtkm::Vec<ComponentType, NumComponents>, MaxSize>
{
public:
  using Type = svtkm::Vec<ComponentType, NumComponents>;

  explicit SVTKM_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    SVTKM_ASSERT(vec.GetNumberOfComponents() <= MaxSize);
  }

  SVTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};

template <typename ComponentType, svtkm::IdComponent MaxSize1, svtkm::IdComponent MaxSize2>
class FastVec<svtkm::VecVariable<ComponentType, MaxSize1>, MaxSize2>
{
public:
  using Type = svtkm::VecVariable<ComponentType, MaxSize1>;

  explicit SVTKM_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    SVTKM_ASSERT(vec.GetNumberOfComponents() <= MaxSize2);
  }

  SVTKM_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};
}
}
} // svtkm::exec::internal

#endif // svtk_m_exec_internal_FastVec_h
