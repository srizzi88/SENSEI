//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagExecObject_h
#define svtk_m_exec_arg_FetchTagExecObject_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>

#include <type_traits>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for execution objects.
///
/// \c FetchTagExecObject is a tag used with the \c Fetch class to retrieve
/// execution objects. For safety, execution objects are read-only. \c
/// FetchTagExecObject is almost always used in conjunction with \c
/// TransportTagExecObject and vice versa.
///
struct FetchTagExecObject
{
};

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagExecObject,
             svtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  using ValueType = ExecObjectType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& svtkmNotUsed(indices),
                 const ExecObjectType& execObject) const
  {
    return execObject;
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_FetchTagExecObject_h
