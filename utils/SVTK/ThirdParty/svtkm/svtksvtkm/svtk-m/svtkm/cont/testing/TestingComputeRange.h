//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingComputeRange_h
#define svtk_m_cont_testing_TestingComputeRange_h

#include <svtkm/Types.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/testing/Testing.h>

// Required for implementation of ArrayRangeCompute for virtual arrays
#include <svtkm/cont/ArrayRangeCompute.hxx>

#include <algorithm>
#include <iostream>
#include <random>

namespace svtkm
{
namespace cont
{
namespace testing
{

using CustomTypeList = svtkm::List<svtkm::Vec<Int32, 3>,
                                  svtkm::Vec<Int64, 3>,
                                  svtkm::Vec<Float32, 3>,
                                  svtkm::Vec<Float64, 3>,
                                  svtkm::Vec<Int32, 9>,
                                  svtkm::Vec<Int64, 9>,
                                  svtkm::Vec<Float32, 9>,
                                  svtkm::Vec<Float64, 9>>;

template <typename DeviceAdapterTag>
class TestingComputeRange
{
private:
  template <typename T>
  static void TestScalarField()
  {
    const svtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(data, data + nvals, urng);
    auto field =
      svtkm::cont::make_Field("TestField", svtkm::cont::Field::Association::POINTS, data, nvals);

    svtkm::Range result;
    field.GetRange(&result);

    std::cout << result << std::endl;
    SVTKM_TEST_ASSERT((test_equal(result.Min, -5.0) && test_equal(result.Max, 5.0)),
                     "Unexpected scalar field range.");
  }

  template <typename T, svtkm::IdComponent NumberOfComponents>
  static void TestVecField()
  {
    const svtkm::Id nvals = 11;
    T data[nvals] = { 1, 2, 3, 4, 5, -5, -4, -3, -2, -1, 0 };
    svtkm::Vec<T, NumberOfComponents> fieldData[nvals];
    std::random_device rng;
    std::mt19937 urng(rng());
    for (svtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      std::shuffle(data, data + nvals, urng);
      for (svtkm::Id j = 0; j < nvals; ++j)
      {
        fieldData[j][i] = data[j];
      }
    }
    auto field =
      svtkm::cont::make_Field("TestField", svtkm::cont::Field::Association::POINTS, fieldData, nvals);

    svtkm::Range result[NumberOfComponents];
    field.GetRange(result, CustomTypeList());

    for (svtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      SVTKM_TEST_ASSERT((test_equal(result[i].Min, -5.0) && test_equal(result[i].Max, 5.0)),
                       "Unexpected vector field range.");
    }
  }

  static void TestUniformCoordinateField()
  {
    svtkm::cont::CoordinateSystem field("TestField",
                                       svtkm::Id3(10, 20, 5),
                                       svtkm::Vec3f(0.0f, -5.0f, 4.0f),
                                       svtkm::Vec3f(1.0f, 0.5f, 2.0f));

    svtkm::Bounds result = field.GetBounds();

    SVTKM_TEST_ASSERT(test_equal(result.X.Min, 0.0), "Min x wrong.");
    SVTKM_TEST_ASSERT(test_equal(result.X.Max, 9.0), "Max x wrong.");
    SVTKM_TEST_ASSERT(test_equal(result.Y.Min, -5.0), "Min y wrong.");
    SVTKM_TEST_ASSERT(test_equal(result.Y.Max, 4.5), "Max y wrong.");
    SVTKM_TEST_ASSERT(test_equal(result.Z.Min, 4.0), "Min z wrong.");
    SVTKM_TEST_ASSERT(test_equal(result.Z.Max, 12.0), "Max z wrong.");
  }

  struct TestAll
  {
    SVTKM_CONT void operator()() const
    {
      std::cout << "Testing (Int32, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<svtkm::Int32>();
      std::cout << "Testing (Int64, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<svtkm::Int64>();
      std::cout << "Testing (Float32, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<svtkm::Float32>();
      std::cout << "Testing (Float64, 1)..." << std::endl;
      TestingComputeRange::TestScalarField<svtkm::Float64>();

      std::cout << "Testing (Int32, 3)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Int32, 3>();
      std::cout << "Testing (Int64, 3)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Int64, 3>();
      std::cout << "Testing (Float32, 3)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Float32, 3>();
      std::cout << "Testing (Float64, 3)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Float64, 3>();

      std::cout << "Testing (Int32, 9)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Int32, 9>();
      std::cout << "Testing (Int64, 9)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Int64, 9>();
      std::cout << "Testing (Float32, 9)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Float32, 9>();
      std::cout << "Testing (Float64, 9)..." << std::endl;
      TestingComputeRange::TestVecField<svtkm::Float64, 9>();

      std::cout << "Testing UniformPointCoords..." << std::endl;
      TestingComputeRange::TestUniformCoordinateField();
    }
  };

public:
  static SVTKM_CONT int Run(int argc, char* argv[])
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return svtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif //svtk_m_cont_testing_TestingComputeRange_h
