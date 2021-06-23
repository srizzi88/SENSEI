//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_RuntimeDeviceInformation_h
#define svtk_m_cont_RuntimeDeviceInformation_h

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{
namespace cont
{

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is very important
/// for device adapters where a physical hardware requirements such as a GPU
/// or a Accelerator Card is needed for support to exist.
///
///
class SVTKM_CONT_EXPORT RuntimeDeviceInformation
{
public:
  /// Returns the name corresponding to the device adapter id. If @a id is
  /// not recognized, `InvalidDeviceId` is returned. Queries for a
  /// name are all case-insensitive.
  SVTKM_CONT
  DeviceAdapterNameType GetName(DeviceAdapterId id) const;

  /// Returns the id corresponding to the device adapter name. If @a name is
  /// not recognized, DeviceAdapterTagUndefined is returned.
  SVTKM_CONT
  DeviceAdapterId GetId(DeviceAdapterNameType name) const;

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  SVTKM_CONT
  bool Exists(DeviceAdapterId id) const;
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_RuntimeDeviceInformation_h
