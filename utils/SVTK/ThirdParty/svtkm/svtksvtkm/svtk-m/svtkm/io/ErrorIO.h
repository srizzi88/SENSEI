//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_ErrorIO_h
#define svtk_m_io_ErrorIO_h

#include <svtkm/cont/Error.h>

namespace svtkm
{
namespace io
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKM_ALWAYS_EXPORT ErrorIO : public svtkm::cont::Error
{
public:
  ErrorIO() {}
  ErrorIO(const std::string message)
    : Error(message)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace svtkm::io

#endif //svtk_m_io_ErrorIO_h
