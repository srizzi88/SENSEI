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

//include all backends
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

template <bool>
struct DoesExist;

template <typename DeviceAdapterTag>
void detect_if_exists(DeviceAdapterTag tag)
{
  using DeviceAdapterTraits = svtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
  std::cout << "testing runtime support for " << DeviceAdapterTraits::GetName() << std::endl;
  DoesExist<tag.IsEnabled> exist;
  exist.Exist(tag);
}

template <>
struct DoesExist<false>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {

    //runtime information for this device should return false
    svtkm::cont::RuntimeDeviceInformation runtime;
    SVTKM_TEST_ASSERT(runtime.Exists(DeviceAdapterTag()) == false,
                     "A backend with zero compile time support, can't have runtime support");
  }

  void Exist(svtkm::cont::DeviceAdapterTagCuda) const
  {
    //Since we are in a C++ compilation unit the Device Adapter
    //trait should be false. But CUDA could still be enabled.
    //That is why we check SVTKM_ENABLE_CUDA.
    svtkm::cont::RuntimeDeviceInformation runtime;
#ifdef SVTKM_ENABLE_CUDA
    SVTKM_TEST_ASSERT(runtime.Exists(svtkm::cont::DeviceAdapterTagCuda()) == true,
                     "with cuda backend enabled, runtime support should be enabled");
#else
    SVTKM_TEST_ASSERT(runtime.Exists(svtkm::cont::DeviceAdapterTagCuda()) == false,
                     "with cuda backend disabled, runtime support should be disabled");
#endif
  }
};

template <>
struct DoesExist<true>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {
    //runtime information for this device should return true
    svtkm::cont::RuntimeDeviceInformation runtime;
    SVTKM_TEST_ASSERT(runtime.Exists(DeviceAdapterTag()) == true,
                     "A backend with compile time support, should have runtime support");
  }
};

void Detection()
{
  using SerialTag = ::svtkm::cont::DeviceAdapterTagSerial;
  using OpenMPTag = ::svtkm::cont::DeviceAdapterTagOpenMP;
  using TBBTag = ::svtkm::cont::DeviceAdapterTagTBB;
  using CudaTag = ::svtkm::cont::DeviceAdapterTagCuda;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  detect_if_exists(SerialTag());
  detect_if_exists(OpenMPTag());
  detect_if_exists(CudaTag());
  detect_if_exists(TBBTag());
}

} // anonymous namespace

int UnitTestRuntimeDeviceInformation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(Detection, argc, argv);
}
