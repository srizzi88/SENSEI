//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Bitset_h
#define svtk_m_Bitset_h

#include <cassert>
#include <limits>
#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{

/// \brief A bitmap to serve different needs.
/// Ex. Editing particular bits in a byte(s), checkint if particular bit values
/// are present or not. Once Cuda supports std::bitset, we should use the
/// standard one if possible. Additional cast in logical operations are required
/// to avoid compiler warnings when using 16 or 8 bit MaskType.
template <typename MaskType>
struct Bitset
{
  SVTKM_EXEC_CONT void set(svtkm::Id bitIndex)
  {
    this->Mask = static_cast<MaskType>(this->Mask | (static_cast<MaskType>(1) << bitIndex));
  }

  SVTKM_EXEC_CONT void set(svtkm::Id bitIndex, bool val)
  {
    if (val)
      this->set(bitIndex);
    else
      this->reset(bitIndex);
  }

  SVTKM_EXEC_CONT void reset(svtkm::Id bitIndex)
  {
    this->Mask = static_cast<MaskType>(this->Mask & ~(static_cast<MaskType>(1) << bitIndex));
  }

  SVTKM_EXEC_CONT void toggle(svtkm::Id bitIndex)
  {
    this->Mask = this->Mask ^ (static_cast<MaskType>(0) << bitIndex);
  }

  SVTKM_EXEC_CONT bool test(svtkm::Id bitIndex) const
  {
    return ((this->Mask & (static_cast<MaskType>(1) << bitIndex)) != 0);
  }

private:
  MaskType Mask = 0;
};

} // namespace svtkm

#endif //svtk_m_Bitset_h
