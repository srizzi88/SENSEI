//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ErrorFilterExecution_h
#define svtk_m_cont_ErrorFilterExecution_h

#include <svtkm/cont/Error.h>

namespace svtkm
{
namespace cont
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is primarily intended to filters to throw in the control
/// environment to indicate an execution failure due to misconfiguration e.g.
/// incorrect parameters, etc. This is a device independent exception i.e. when
/// thrown, unlike most other exceptions, SVTK-m will not try to re-execute the
/// filter on another available device.
class SVTKM_ALWAYS_EXPORT ErrorFilterExecution : public svtkm::cont::Error
{
public:
  ErrorFilterExecution(const std::string& message)
    : Error(message, /*is_device_independent=*/true)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace svtkm::cont

#endif
