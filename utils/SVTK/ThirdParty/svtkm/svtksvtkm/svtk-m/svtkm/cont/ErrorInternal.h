//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ErrorInternal_h
#define svtk_m_cont_ErrorInternal_h

#include <svtkm/cont/Error.h>

namespace svtkm
{
namespace cont
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when SVTKm detects an internal state that should never
/// be reached. This error usually indicates a bug in svtkm or, at best, SVTKm
/// failed to detect an invalid input it should have.
///
class SVTKM_ALWAYS_EXPORT ErrorInternal : public Error
{
public:
  ErrorInternal(const std::string& message)
    : Error(message)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ErrorInternal_h
