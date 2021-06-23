//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ErrorBadAllocation_h
#define svtk_m_cont_ErrorBadAllocation_h

#include <svtkm/cont/Error.h>

namespace svtkm
{
namespace cont
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when SVTK-m attempts to manipulate memory that it should
/// not.
///
class SVTKM_ALWAYS_EXPORT ErrorBadAllocation : public Error
{
public:
  ErrorBadAllocation(const std::string& message)
    : Error(message)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ErrorBadAllocation_h
