//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/RuntimeDeviceInformation.h>

#include <svtkm/List.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterList.h>
#include <svtkm/cont/DeviceAdapterTag.h>

//Bring in each device adapters runtime class
#include <svtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <svtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <svtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>
#include <svtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>

#include <cctype> //for tolower

namespace
{
struct SVTKM_NEVER_EXPORT InitializeDeviceNames
{
  svtkm::cont::DeviceAdapterNameType* Names;
  svtkm::cont::DeviceAdapterNameType* LowerCaseNames;

  SVTKM_CONT
  InitializeDeviceNames(svtkm::cont::DeviceAdapterNameType* names,
                        svtkm::cont::DeviceAdapterNameType* lower)
    : Names(names)
    , LowerCaseNames(lower)
  {
    std::fill_n(this->Names, SVTKM_MAX_DEVICE_ADAPTER_ID, "InvalidDeviceId");
    std::fill_n(this->LowerCaseNames, SVTKM_MAX_DEVICE_ADAPTER_ID, "invaliddeviceid");
  }

  template <typename Device>
  SVTKM_CONT void operator()(Device device)
  {
    auto lowerCaseFunc = [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    };

    auto id = device.GetValue();

    if (id > 0 && id < SVTKM_MAX_DEVICE_ADAPTER_ID)
    {
      auto name = svtkm::cont::DeviceAdapterTraits<Device>::GetName();
      this->Names[id] = name;
      std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);
      this->LowerCaseNames[id] = name;
    }
  }
};

struct SVTKM_NEVER_EXPORT RuntimeDeviceInformationFunctor
{
  bool Exists = false;
  template <typename DeviceAdapter>
  SVTKM_CONT void operator()(DeviceAdapter, svtkm::cont::DeviceAdapterId device)
  {
    if (DeviceAdapter() == device)
    {
      this->Exists = svtkm::cont::DeviceAdapterRuntimeDetector<DeviceAdapter>().Exists();
    }
  }
};
}

namespace svtkm
{
namespace cont
{
namespace detail
{

class RuntimeDeviceNames
{
public:
  static const DeviceAdapterNameType& GetDeviceName(svtkm::Int8 id)
  {
    return Instance().DeviceNames[id];
  }

  static const DeviceAdapterNameType& GetLowerCaseDeviceName(svtkm::Int8 id)
  {
    return Instance().LowerCaseDeviceNames[id];
  }

private:
  static const RuntimeDeviceNames& Instance()
  {
    static RuntimeDeviceNames instance;
    return instance;
  }

  RuntimeDeviceNames()
  {
    InitializeDeviceNames functor(DeviceNames, LowerCaseDeviceNames);
    svtkm::ListForEach(functor, SVTKM_DEFAULT_DEVICE_ADAPTER_LIST());
  }

  friend struct InitializeDeviceNames;

  DeviceAdapterNameType DeviceNames[SVTKM_MAX_DEVICE_ADAPTER_ID];
  DeviceAdapterNameType LowerCaseDeviceNames[SVTKM_MAX_DEVICE_ADAPTER_ID];
};
}

SVTKM_CONT
DeviceAdapterNameType RuntimeDeviceInformation::GetName(DeviceAdapterId device) const
{
  const auto id = device.GetValue();

  if (device.IsValueValid())
  {
    return detail::RuntimeDeviceNames::GetDeviceName(id);
  }
  else if (id == SVTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    return svtkm::cont::DeviceAdapterTraits<svtkm::cont::DeviceAdapterTagUndefined>::GetName();
  }
  else if (id == SVTKM_DEVICE_ADAPTER_ANY)
  {
    return svtkm::cont::DeviceAdapterTraits<svtkm::cont::DeviceAdapterTagAny>::GetName();
  }

  // Deviceis invalid:
  return detail::RuntimeDeviceNames::GetDeviceName(0);
}

SVTKM_CONT
DeviceAdapterId RuntimeDeviceInformation::GetId(DeviceAdapterNameType name) const
{
  // The GetDeviceAdapterId call is case-insensitive so transform the name to be lower case
  // as that is how we cache the case-insensitive version.
  auto lowerCaseFunc = [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  };
  std::transform(name.begin(), name.end(), name.begin(), lowerCaseFunc);

  //lower-case the name here
  if (name == "any")
  {
    return svtkm::cont::DeviceAdapterTagAny{};
  }
  else if (name == "undefined")
  {
    return svtkm::cont::DeviceAdapterTagUndefined{};
  }

  for (svtkm::Int8 id = 0; id < SVTKM_MAX_DEVICE_ADAPTER_ID; ++id)
  {
    if (name == detail::RuntimeDeviceNames::GetLowerCaseDeviceName(id))
    {
      return svtkm::cont::make_DeviceAdapterId(id);
    }
  }

  return svtkm::cont::DeviceAdapterTagUndefined{};
}


SVTKM_CONT
bool RuntimeDeviceInformation::Exists(DeviceAdapterId id) const
{
  if (id == svtkm::cont::DeviceAdapterTagAny{})
  {
    return true;
  }

  RuntimeDeviceInformationFunctor functor;
  svtkm::ListForEach(functor, SVTKM_DEFAULT_DEVICE_ADAPTER_LIST(), id);
  return functor.Exists;
}
}
} // namespace svtkm::cont
