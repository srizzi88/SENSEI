//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ErrorBadType_h
#define svtk_m_cont_ErrorBadType_h

#include <svtkm/cont/Error.h>

namespace svtkm
{
namespace cont
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when SVTK-m encounters data of a type that is
/// incompatible with the current operation.
///
class SVTKM_ALWAYS_EXPORT ErrorBadType : public Error
{
public:
  ErrorBadType(const std::string& message)
    : Error(message, true)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END

/// Throws an ErrorBadType exception with the following message:
/// Cast failed: \c baseType --> \c derivedType".
/// This is generally caused by asking for a casting of a VariantArrayHandle
/// with an insufficient type list.
//
SVTKM_CONT_EXPORT void throwFailedDynamicCast(const std::string& baseType,
                                             const std::string& derivedType);
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ErrorBadType_h
