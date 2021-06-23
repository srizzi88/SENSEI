//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This test makes sure that the algorithms specified in
// DeviceAdapterAlgorithmGeneral.h are working correctly. It does this by
// creating a test device adapter that uses the serial device adapter for the
// base schedule/scan/sort algorithms and using the general algorithms for
// everything else. Because this test is based of the serial device adapter,
// make sure that UnitTestDeviceAdapterSerial is working before trying to debug
// this one.
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/internal/AtomicInterfaceControl.h>
#include <svtkm/cont/internal/AtomicInterfaceExecution.h>
#include <svtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <svtkm/cont/internal/VirtualObjectTransferShareWithControl.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/TestingDeviceAdapter.h>

SVTKM_VALID_DEVICE_ADAPTER(TestAlgorithmGeneral, 7);

namespace svtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : svtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>,
      svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
private:
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>;

  using DeviceAdapterTagTestAlgorithmGeneral = svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral;

public:
  template <class Functor>
  SVTKM_CONT static void Schedule(Functor functor, svtkm::Id numInstances)
  {
    Algorithm::Schedule(functor, numInstances);
  }

  template <class Functor>
  SVTKM_CONT static void Schedule(Functor functor, svtkm::Id3 rangeMax)
  {
    Algorithm::Schedule(functor, rangeMax);
  }

  SVTKM_CONT static void Synchronize() { Algorithm::Synchronize(); }
};

template <>
class DeviceAdapterRuntimeDetector<svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
public:
  /// Returns true as the General Algorithm Device can always be used.
  SVTKM_CONT bool Exists() const { return true; }
};


namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : public svtkm::cont::internal::ArrayManagerExecution<T,
                                                       StorageTag,
                                                       svtkm::cont::DeviceAdapterTagSerial>
{
public:
  using Superclass =
    svtkm::cont::internal::ArrayManagerExecution<T, StorageTag, svtkm::cont::DeviceAdapterTagSerial>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;

  ArrayManagerExecution(svtkm::cont::internal::Storage<T, StorageTag>* storage)
    : Superclass(storage)
  {
  }
};

template <>
class AtomicInterfaceExecution<DeviceAdapterTagTestAlgorithmGeneral> : public AtomicInterfaceControl
{
};

template <typename TargetClass>
struct VirtualObjectTransfer<TargetClass, svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral> final
  : public VirtualObjectTransferShareWithControl<TargetClass>
{
  using VirtualObjectTransferShareWithControl<TargetClass>::VirtualObjectTransferShareWithControl;
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagTestAlgorithmGeneral>
  : public ExecutionPortalFactoryBasicShareWithControl<T>
{
  using Superclass = ExecutionPortalFactoryBasicShareWithControl<T>;

  using Superclass::CreatePortal;
  using Superclass::CreatePortalConst;
  using typename Superclass::PortalConstType;
  using typename Superclass::PortalType;
  using typename Superclass::ValueType;
};

template <>
struct ExecutionArrayInterfaceBasic<DeviceAdapterTagTestAlgorithmGeneral>
  : public ExecutionArrayInterfaceBasicShareWithControl
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicShareWithControl::ExecutionArrayInterfaceBasicShareWithControl;

  SVTKM_CONT
  DeviceAdapterId GetDeviceId() const final { return DeviceAdapterTagTestAlgorithmGeneral{}; }
};
}
}
} // namespace svtkm::cont::internal

int UnitTestDeviceAdapterAlgorithmGeneral(int argc, char* argv[])
{
  //need to enable DeviceAdapterTagTestAlgorithmGeneral as it
  //is not part of the default set of devices
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ResetDevice(svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral{});

  return svtkm::cont::testing::TestingDeviceAdapter<
    svtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>::Run(argc, argv);
}
