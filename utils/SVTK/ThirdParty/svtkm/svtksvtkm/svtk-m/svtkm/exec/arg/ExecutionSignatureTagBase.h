//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ExecutionSignatureTagBase_h
#define svtk_m_exec_arg_ExecutionSignatureTagBase_h

#include <svtkm/StaticAssert.h>
#include <svtkm/internal/ExportMacros.h>

#include <type_traits>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief The base class for all tags used in an \c ExecutionSignature.
///
/// If a new \c ExecutionSignature tag is created, it must be derived from this
/// class in some way. This helps identify \c ExecutionSignature tags in the \c
/// SVTKM_IS_EXECUTION_SIGNATURE_TAG macro and allows checking the validity of
/// an \c ExecutionSignature.
///
/// In addition to inheriting from this base class, an \c ExecutionSignature
/// tag must define a \c static \c const \c svtkm::IdComponent named \c INDEX
/// that points to a parameter in the \c ControlSignature and a \c typedef
/// named \c AspectTag that defines the aspect of the fetch.
///
struct ExecutionSignatureTagBase
{
};

namespace internal
{

template <typename ExecutionSignatureTag>
struct ExecutionSignatureTagCheck
{
  static constexpr bool Valid =
    std::is_base_of<svtkm::exec::arg::ExecutionSignatureTagBase, ExecutionSignatureTag>::value;
};

} // namespace internal

/// Checks that the argument is a proper tag for an \c ExecutionSignature. This
/// is a handy concept check when modifying tags or dispatching to make sure
/// that a template argument is actually an \c ExecutionSignature tag. (You can
/// get weird errors elsewhere in the code when a mistake is made.)
///
#define SVTKM_IS_EXECUTION_SIGNATURE_TAG(tag)                                                       \
  SVTKM_STATIC_ASSERT_MSG(::svtkm::exec::arg::internal::ExecutionSignatureTagCheck<tag>::Valid,      \
                         "Provided a type that is not a valid ExecutionSignature tag.")
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_ExecutionSignatureTagBase_h
