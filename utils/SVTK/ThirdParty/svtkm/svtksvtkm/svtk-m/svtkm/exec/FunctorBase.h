//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_FunctorBase_h
#define svtk_m_exec_FunctorBase_h

#include <svtkm/Types.h>

#include <svtkm/exec/internal/ErrorMessageBuffer.h>

#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace exec
{

/// Base class for all user worklets invoked in the execution environment from a
/// call to svtkm::cont::DeviceAdapterAlgorithm::Schedule.
///
/// This class contains a public method named RaiseError that can be called in
/// the execution environment to signal a problem.
///
class SVTKM_ALWAYS_EXPORT FunctorBase
{
public:
  SVTKM_EXEC_CONT
  FunctorBase()
    : ErrorMessage()
  {
  }

  SVTKM_EXEC
  void RaiseError(const char* message) const { this->ErrorMessage.RaiseError(message); }

  /// Set the error message buffer so that running algorithms can report
  /// errors. This is supposed to be set by the dispatcher. This method may be
  /// replaced as the execution semantics change.
  ///
  SVTKM_CONT
  void SetErrorMessageBuffer(const svtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->ErrorMessage = buffer;
  }

private:
  svtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_FunctorBase_h
