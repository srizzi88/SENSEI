//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_BasicArg_h
#define svtk_m_exec_arg_BasicArg_h

#include <svtkm/Types.h>

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief The underlying tag for basic \c ExecutionSignature arguments.
///
/// The basic \c ExecutionSignature arguments of _1, _2, etc. are all
/// subclasses of \c BasicArg. They all make available the components of
/// this class.
///
template <svtkm::IdComponent ControlSignatureIndex>
struct BasicArg : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = ControlSignatureIndex;
  using AspectTag = svtkm::exec::arg::AspectTagDefault;
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_BasicArg_h
