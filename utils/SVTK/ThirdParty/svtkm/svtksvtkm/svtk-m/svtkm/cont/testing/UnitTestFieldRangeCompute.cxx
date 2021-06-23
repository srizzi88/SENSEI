//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/FieldRangeCompute.h>
#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

static unsigned int uid = 1;

template <typename T>
svtkm::cont::ArrayHandle<T> CreateArray(T min, T max, svtkm::Id numVals, svtkm::TypeTraitsScalarTag)
{
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  svtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  std::generate(svtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                svtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}

template <typename T>
svtkm::cont::ArrayHandle<T> CreateArray(const T& min,
                                       const T& max,
                                       svtkm::Id numVals,
                                       svtkm::TypeTraitsVectorTag)
{
  constexpr int size = T::NUM_COMPONENTS;
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis[size];
  for (int cc = 0; cc < size; ++cc)
  {
    dis[cc] = std::uniform_real_distribution<double>(static_cast<double>(min[cc]),
                                                     static_cast<double>(max[cc]));
  }
  svtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);
  std::generate(svtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                svtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() {
                  T val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<typename T::ComponentType>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}

static constexpr svtkm::Id ARRAY_SIZE = 1025;

template <typename ValueType>
void Validate(const svtkm::cont::ArrayHandle<svtkm::Range>& ranges,
              const ValueType& min,
              const ValueType& max)
{
  SVTKM_TEST_ASSERT(ranges.GetNumberOfValues() == 1, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  auto range = portal.Get(0);
  std::cout << "  expecting [" << min << ", " << max << "], got [" << range.Min << ", " << range.Max
            << "]" << std::endl;
  SVTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<ValueType>(min) &&
                     range.Max <= static_cast<ValueType>(max),
                   "Got wrong range.");
}

template <typename T, int size>
void Validate(const svtkm::cont::ArrayHandle<svtkm::Range>& ranges,
              const svtkm::Vec<T, size>& min,
              const svtkm::Vec<T, size>& max)
{
  SVTKM_TEST_ASSERT(ranges.GetNumberOfValues() == size, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  for (int cc = 0; cc < size; ++cc)
  {
    auto range = portal.Get(cc);
    std::cout << "  [0] expecting [" << min[cc] << ", " << max[cc] << "], got [" << range.Min
              << ", " << range.Max << "]" << std::endl;
    SVTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<T>(min[cc]) &&
                       range.Max <= static_cast<T>(max[cc]),
                     "Got wrong range.");
  }
}

template <typename ValueType>
void TryRangeComputeDS(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (dataset): " << svtkm::testing::TypeName<ValueType>::Name() << std::endl;
  // let's create a dummy dataset with a bunch of fields.
  svtkm::cont::DataSet dataset;
  svtkm::cont::DataSetFieldAdd::AddPointField(
    dataset,
    "pointvar",
    CreateArray(min, max, ARRAY_SIZE, typename svtkm::TypeTraits<ValueType>::DimensionalityTag()));

  svtkm::cont::ArrayHandle<svtkm::Range> ranges = svtkm::cont::FieldRangeCompute(dataset, "pointvar");
  Validate(ranges, min, max);
}

template <typename ValueType>
void TryRangeComputePDS(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (PartitionedDataSet): " << svtkm::testing::TypeName<ValueType>::Name()
            << std::endl;

  svtkm::cont::PartitionedDataSet mb;
  for (int cc = 0; cc < 5; cc++)
  {
    // let's create a dummy dataset with a bunch of fields.
    svtkm::cont::DataSet dataset;
    svtkm::cont::DataSetFieldAdd::AddPointField(
      dataset,
      "pointvar",
      CreateArray(min, max, ARRAY_SIZE, typename svtkm::TypeTraits<ValueType>::DimensionalityTag()));
    mb.AppendPartition(dataset);
  }

  svtkm::cont::ArrayHandle<svtkm::Range> ranges = svtkm::cont::FieldRangeCompute(mb, "pointvar");
  Validate(ranges, min, max);
}

static void TestFieldRangeCompute()
{
  // init random seed.
  std::srand(100);

  TryRangeComputeDS<svtkm::Float64>(0, 1000);
  TryRangeComputeDS<svtkm::Int32>(-1024, 1024);
  TryRangeComputeDS<svtkm::Vec3f_32>(svtkm::make_Vec(1024, 0, -1024),
                                    svtkm::make_Vec(2048, 2048, 2048));
  TryRangeComputePDS<svtkm::Float64>(0, 1000);
  TryRangeComputePDS<svtkm::Int32>(-1024, 1024);
  TryRangeComputePDS<svtkm::Vec3f_32>(svtkm::make_Vec(1024, 0, -1024),
                                     svtkm::make_Vec(2048, 2048, 2048));
};

int UnitTestFieldRangeCompute(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestFieldRangeCompute, argc, argv);
}
