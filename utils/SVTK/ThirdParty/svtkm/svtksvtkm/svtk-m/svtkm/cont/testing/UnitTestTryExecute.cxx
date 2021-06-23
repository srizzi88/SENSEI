//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/Error.h>
#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorBadDevice.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/Testing.h>

#include <exception>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

class ErrorDeviceIndependent : public svtkm::cont::Error
{
public:
  ErrorDeviceIndependent(const std::string& msg)
    : svtkm::cont::Error(msg, true)
  {
  }
};

class ErrorDeviceDependent : public svtkm::cont::Error
{
public:
  ErrorDeviceDependent(const std::string& msg)
    : svtkm::cont::Error(msg, false)
  {
  }
};

struct TryExecuteTestFunctor
{
  svtkm::IdComponent NumCalls;

  SVTKM_CONT
  TryExecuteTestFunctor()
    : NumCalls(0)
  {
  }

  template <typename Device>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::cont::ArrayHandle<svtkm::FloatDefault>& in,
                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>& out)
  {
    using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<Device>;
    Algorithm::Copy(in, out);
    this->NumCalls++;
    return true;
  }
};

template <typename ExceptionT>
struct TryExecuteTestErrorFunctor
{
  template <typename Device>
  SVTKM_CONT bool operator()(Device)
  {
    throw ExceptionT("Test message");
  }
};

template <typename DeviceList>
void TryExecuteTests(DeviceList, bool expectSuccess)
{
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> inArray;
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> outArray;

  inArray.Allocate(ARRAY_SIZE);
  SetPortal(inArray.GetPortalControl());

  TryExecuteTestFunctor functor;

  bool result = svtkm::cont::TryExecute(functor, DeviceList(), inArray, outArray);

  if (expectSuccess)
  {
    SVTKM_TEST_ASSERT(result, "Call returned failure when expected success.");
    SVTKM_TEST_ASSERT(functor.NumCalls == 1, "Bad number of calls");
    CheckPortal(outArray.GetPortalConstControl());
  }
  else
  {
    SVTKM_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }

  //verify the ability to pass rvalue functors
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> outArray2;
  result = svtkm::cont::TryExecute(TryExecuteTestFunctor(), DeviceList(), inArray, outArray2);
  if (expectSuccess)
  {
    SVTKM_TEST_ASSERT(result, "Call returned failure when expected success.");
    CheckPortal(outArray2.GetPortalConstControl());
  }
  else
  {
    SVTKM_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }
}

struct EdgeCaseFunctor
{
  template <typename DeviceList>
  bool operator()(DeviceList, int, float, bool) const
  {
    return true;
  }
  template <typename DeviceList>
  bool operator()(DeviceList) const
  {
    return true;
  }
};

void TryExecuteAllEdgeCases()
{
  using ValidDevice = svtkm::cont::DeviceAdapterTagSerial;
  using SingleValidList = svtkm::List<ValidDevice>;

  std::cout << "TryExecute no Runtime, no Device, no parameters." << std::endl;
  svtkm::cont::TryExecute(EdgeCaseFunctor());

  std::cout << "TryExecute no Runtime, no Device, with parameters." << std::endl;
  svtkm::cont::TryExecute(EdgeCaseFunctor(), int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute with Device, no parameters." << std::endl;
  svtkm::cont::TryExecute(EdgeCaseFunctor(), SingleValidList());

  std::cout << "TryExecute with Device, with parameters." << std::endl;
  svtkm::cont::TryExecute(
    EdgeCaseFunctor(), SingleValidList(), int{ 42 }, float{ 3.14f }, bool{ true });
}

template <typename ExceptionType>
void RunErrorTest(bool shouldFail, bool shouldThrow, bool shouldDisable)
{
  using Device = svtkm::cont::DeviceAdapterTagSerial;
  using Functor = TryExecuteTestErrorFunctor<ExceptionType>;

  // Initialize this one to what we expect -- it won't get set if we throw.
  bool succeeded = !shouldFail;
  bool threw = false;
  bool disabled = false;

  svtkm::cont::ScopedRuntimeDeviceTracker scopedTracker(Device{});

  try
  {
    succeeded = svtkm::cont::TryExecute(Functor{});
    threw = false;
  }
  catch (...)
  {
    threw = true;
  }

  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  disabled = !tracker.CanRunOn(Device{});

  std::cout << "Failed: " << !succeeded << " "
            << "Threw: " << threw << " "
            << "Disabled: " << disabled << "\n"
            << std::endl;

  SVTKM_TEST_ASSERT(shouldFail == !succeeded, "TryExecute return status incorrect.");
  SVTKM_TEST_ASSERT(threw == shouldThrow, "TryExecute throw behavior incorrect.");
  SVTKM_TEST_ASSERT(disabled == shouldDisable, "TryExecute device-disabling behavior incorrect.");
}

void TryExecuteErrorTests()
{
  std::cout << "Test ErrorBadAllocation." << std::endl;
  RunErrorTest<svtkm::cont::ErrorBadAllocation>(true, false, true);

  std::cout << "Test ErrorBadDevice." << std::endl;
  RunErrorTest<svtkm::cont::ErrorBadDevice>(true, false, true);

  std::cout << "Test ErrorBadType." << std::endl;
  RunErrorTest<svtkm::cont::ErrorBadType>(true, false, false);

  std::cout << "Test ErrorBadValue." << std::endl;
  RunErrorTest<svtkm::cont::ErrorBadValue>(true, true, false);

  std::cout << "Test custom svtkm Error (dev indep)." << std::endl;
  RunErrorTest<ErrorDeviceIndependent>(true, true, false);

  std::cout << "Test custom svtkm Error (dev dep)." << std::endl;
  RunErrorTest<ErrorDeviceDependent>(true, false, false);

  std::cout << "Test std::exception." << std::endl;
  RunErrorTest<std::runtime_error>(true, false, false);

  std::cout << "Test throw non-exception." << std::endl;
  RunErrorTest<std::string>(true, false, false);
}

static void Run()
{
  using ValidDevice = svtkm::cont::DeviceAdapterTagSerial;
  using InvalidDevice = svtkm::cont::DeviceAdapterTagUndefined;

  TryExecuteAllEdgeCases();

  std::cout << "Try a list with a single entry." << std::endl;
  using SingleValidList = svtkm::List<ValidDevice>;
  TryExecuteTests(SingleValidList(), true);

  std::cout << "Try a list with two valid devices." << std::endl;
  using DoubleValidList = svtkm::List<ValidDevice, ValidDevice>;
  TryExecuteTests(DoubleValidList(), true);

  std::cout << "Try a list with only invalid device." << std::endl;
  using SingleInvalidList = svtkm::List<InvalidDevice>;
  TryExecuteTests(SingleInvalidList(), false);

  std::cout << "Try a list with an invalid and valid device." << std::endl;
  using InvalidAndValidList = svtkm::List<InvalidDevice, ValidDevice>;
  TryExecuteTests(InvalidAndValidList(), true);

  TryExecuteErrorTests();
}

} // anonymous namespace

int UnitTestTryExecute(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(Run, argc, argv);
}
