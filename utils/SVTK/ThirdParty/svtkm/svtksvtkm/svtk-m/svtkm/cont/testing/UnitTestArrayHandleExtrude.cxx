//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================



#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/ArrayHandleExtrudeCoords.h>
#include <svtkm/cont/testing/Testing.h>


namespace
{
//std::vector<svtkm::Vec<svtkm::Float32,3>> points_rz = { svtkm::Vec<svtkm::Float32,3>(1.72485139f, 0.020562f,   1.73493571f),
//                                svtkm::Vec<svtkm::Float32,3>(0.02052826f, 1.73478011f, 0.02299051f )}; //really a vec<float,2>
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<float> correct_x_coords = {
  1.72485139f,      1.73493571f,      1.73478011f,      1.21965411f,  1.22678481f,  1.22667478f,
  1.05616686e-16f,  1.06234173e-16f,  1.06224646e-16f,  -1.21965411f, -1.22678481f, -1.22667478f,
  -1.72485139f,     -1.73493571f,     -1.73478011f,     -1.21965411f, -1.22678481f, -1.22667478f,
  -3.16850059e-16f, -3.18702520e-16f, -3.18673937e-16f, 1.21965411f,  1.22678481f,  1.22667478f
};
std::vector<float> correct_y_coords = { 0.0f,
                                        0.0f,
                                        0.0f,
                                        1.21965411f,
                                        1.22678481f,
                                        1.22667478f,
                                        1.72485139f,
                                        1.73493571f,
                                        1.73478011f,
                                        1.21965411f,
                                        1.22678481f,
                                        1.22667478f,
                                        2.11233373e-16f,
                                        2.12468346e-16f,
                                        2.12449291e-16f,
                                        -1.21965411f,
                                        -1.22678481f,
                                        -1.22667478f,
                                        -1.72485139f,
                                        -1.73493571f,
                                        -1.73478011f,
                                        -1.21965411f,
                                        -1.22678481f,
                                        -1.22667478f };
std::vector<float> correct_z_coords = { 0.020562f,   0.02052826f, 0.02299051f, 0.020562f,
                                        0.02052826f, 0.02299051f, 0.020562f,   0.02052826f,
                                        0.02299051f, 0.020562f,   0.02052826f, 0.02299051f,
                                        0.020562f,   0.02052826f, 0.02299051f, 0.020562f,
                                        0.02052826f, 0.02299051f, 0.020562f,   0.02052826f,
                                        0.02299051f, 0.020562f,   0.02052826f, 0.02299051f };

struct CopyValue : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  template <typename T>
  T&& operator()(T&& t) const
  {
    return std::forward<T>(t);
  }
};

template <typename T, typename S>
void verify_results(svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, S> const& handle)
{
  auto portal = handle.GetPortalConstControl();
  SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == static_cast<svtkm::Id>(correct_x_coords.size()),
                   "coordinate portal size is incorrect");

  for (svtkm::Id i = 0; i < handle.GetNumberOfValues(); ++i)
  {
    auto v = portal.Get(i);
    auto e = svtkm::make_Vec(correct_x_coords[static_cast<std::size_t>(i)],
                            correct_y_coords[static_cast<std::size_t>(i)],
                            correct_z_coords[static_cast<std::size_t>(i)]);
    // std::cout << std::setprecision(4) << "computed " << v << " expected " << e << std::endl;
    SVTKM_TEST_ASSERT(test_equal(v, e), "incorrect conversion to Cartesian space");
  }
}


int TestArrayHandleExtrude()
{
  const int numPlanes = 8;

  auto coords = svtkm::cont::make_ArrayHandleExtrudeCoords(
    svtkm::cont::make_ArrayHandle(points_rz), numPlanes, false);

  SVTKM_TEST_ASSERT(coords.GetNumberOfValues() ==
                     static_cast<svtkm::Id>(((points_rz.size() / 2) * numPlanes)),
                   "coordinate size is incorrect");

  // Verify first that control is correct
  verify_results(coords);

  // Verify 1d scheduling by doing a copy to a svtkm::ArrayHandle<Vec3>
  svtkm::cont::ArrayHandle<svtkm::Vec<float, 3>> output1D;
  svtkm::worklet::DispatcherMapField<CopyValue> dispatcher;
  dispatcher.Invoke(coords, output1D);
  verify_results(output1D);

  return 0;
}

} // end namespace anonymous

int UnitTestArrayHandleExtrude(int argc, char* argv[])
{
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::Testing::Run(TestArrayHandleExtrude, argc, argv);
}
