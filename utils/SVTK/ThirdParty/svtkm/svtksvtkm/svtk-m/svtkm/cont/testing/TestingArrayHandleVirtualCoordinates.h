//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h
#define svtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h

#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace cont
{
namespace testing
{

namespace
{

struct CopyWorklet : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  template <typename T>
  SVTKM_EXEC T operator()(const T& in) const
  {
    return in;
  }
};

// A dummy worklet
struct DoubleWorklet : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn in);
  typedef void ExecutionSignature(_1);
  using InputDomain = _1;

  template <typename T>
  SVTKM_EXEC void operator()(T& in) const
  {
    in = in * 2;
  }
};

template <typename T, typename S>
inline void TestVirtualAccess(const svtkm::cont::ArrayHandle<T, S>& in,
                              svtkm::cont::ArrayHandle<T>& out)
{
  svtkm::worklet::DispatcherMapField<CopyWorklet>().Invoke(
    svtkm::cont::ArrayHandleVirtualCoordinates(in), svtkm::cont::ArrayHandleVirtualCoordinates(out));

  SVTKM_TEST_ASSERT(test_equal_portals(in.GetPortalConstControl(), out.GetPortalConstControl()),
                   "Input and output portals don't match");
}


} // anonymous namespace

template <typename DeviceAdapter>
class TestingArrayHandleVirtualCoordinates
{
private:
  using ArrayHandleRectilinearCoords =
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>;



  static void TestAll()
  {
    using PointType = svtkm::Vec3f;
    static constexpr svtkm::Id length = 64;

    svtkm::cont::ArrayHandle<PointType> out;

    std::cout << "Testing basic ArrayHandle as input\n";
    svtkm::cont::ArrayHandle<PointType> a1;
    a1.Allocate(length);
    for (svtkm::Id i = 0; i < length; ++i)
    {
      a1.GetPortalControl().Set(i, TestValue(i, PointType()));
    }
    TestVirtualAccess(a1, out);

    std::cout << "Testing ArrayHandleUniformPointCoordinates as input\n";
    auto t = svtkm::cont::ArrayHandleUniformPointCoordinates(svtkm::Id3(4, 4, 4));
    TestVirtualAccess(t, out);

    std::cout << "Testing ArrayHandleCartesianProduct as input\n";
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> c1, c2, c3;
    c1.Allocate(length);
    c2.Allocate(length);
    c3.Allocate(length);
    for (svtkm::Id i = 0; i < length; ++i)
    {
      auto p = a1.GetPortalConstControl().Get(i);
      c1.GetPortalControl().Set(i, p[0]);
      c2.GetPortalControl().Set(i, p[1]);
      c3.GetPortalControl().Set(i, p[2]);
    }
    TestVirtualAccess(svtkm::cont::make_ArrayHandleCartesianProduct(c1, c2, c3), out);

    std::cout << "Testing resources releasing on ArrayHandleVirtualCoordinates\n";
    svtkm::cont::ArrayHandleVirtualCoordinates virtualC =
      svtkm::cont::ArrayHandleVirtualCoordinates(a1);
    svtkm::worklet::DispatcherMapField<DoubleWorklet>().Invoke(a1);
    virtualC.ReleaseResourcesExecution();
    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == length,
                     "ReleaseResourcesExecution"
                     " should not change the number of values on the Arrayhandle");
    SVTKM_TEST_ASSERT(
      virtualC.GetNumberOfValues() == length,
      "ReleaseResources"
      " should set the number of values on the ArrayHandleVirtualCoordinates to be 0");
    virtualC.ReleaseResources();
    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == 0,
                     "ReleaseResources"
                     " should set the number of values on the Arrayhandle to be 0");
  }

public:
  static int Run(int argc, char* argv[])
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    return svtkm::cont::testing::Testing::Run(TestAll, argc, argv);
  }
};
}
}
} // svtkm::cont::testing


#endif // svtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h
