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
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/FieldRangeGlobalCompute.h>
#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace
{
static unsigned int uid = 1;

#define PRINT_INFO(msg) std::cout << "[" << comm.rank() << ":" << __LINE__ << "] " msg << std::endl;

#define PRINT_INFO_0(msg)                                                                          \
  if (comm.rank() == 0)                                                                            \
  {                                                                                                \
    std::cout << "[" << comm.rank() << ":" << __LINE__ << "] " msg << std::endl;                   \
  }

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
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  SVTKM_TEST_ASSERT(ranges.GetNumberOfValues() == 1, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  auto range = portal.Get(0);
  PRINT_INFO(<< "  expecting [" << min << ", " << max << "], got [" << range.Min << ", "
             << range.Max
             << "]");
  SVTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<ValueType>(min) &&
                     range.Max <= static_cast<ValueType>(max),
                   "Got wrong range.");
}

template <typename T, int size>
void Validate(const svtkm::cont::ArrayHandle<svtkm::Range>& ranges,
              const svtkm::Vec<T, size>& min,
              const svtkm::Vec<T, size>& max)
{
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  SVTKM_TEST_ASSERT(ranges.GetNumberOfValues() == size, "Wrong number of ranges");

  auto portal = ranges.GetPortalConstControl();
  for (int cc = 0; cc < size; ++cc)
  {
    auto range = portal.Get(cc);
    PRINT_INFO(<< "  [" << cc << "] expecting [" << min[cc] << ", " << max[cc] << "], got ["
               << range.Min
               << ", "
               << range.Max
               << "]");
    SVTKM_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<T>(min[cc]) &&
                       range.Max <= static_cast<T>(max[cc]),
                     "Got wrong range.");
  }
}

template <typename ValueType>
void DecomposeRange(ValueType& min, ValueType& max)
{
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  auto delta = (max - min) / static_cast<ValueType>(comm.size());
  min = min + static_cast<ValueType>(comm.rank()) * delta;
  max = (comm.rank() == comm.size() - 1) ? max : min + delta;
}

template <typename T, int size>
void DecomposeRange(svtkm::Vec<T, size>& min, svtkm::Vec<T, size>& max)
{
  for (int cc = 0; cc < size; ++cc)
  {
    DecomposeRange(min[0], max[0]);
  }
}

template <typename ValueType>
void TryRangeGlobalComputeDS(const ValueType& min, const ValueType& max)
{
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO_0("Trying type (dataset): " << svtkm::testing::TypeName<ValueType>::Name());

  // distribute range among all ranks, so we can confirm reduction works.
  ValueType lmin = min, lmax = max;
  DecomposeRange(lmin, lmax);

  PRINT_INFO("gmin=" << min << ", gmax=" << max << " lmin=" << lmin << ", lmax=" << lmax);

  // let's create a dummy dataset with a bunch of fields.
  svtkm::cont::DataSet dataset;
  svtkm::cont::DataSetFieldAdd::AddPointField(
    dataset,
    "pointvar",
    CreateArray(lmin, lmax, ARRAY_SIZE, typename svtkm::TypeTraits<ValueType>::DimensionalityTag()));

  svtkm::cont::ArrayHandle<svtkm::Range> ranges =
    svtkm::cont::FieldRangeGlobalCompute(dataset, "pointvar");
  Validate(ranges, min, max);
}

template <typename ValueType>
void TryRangeGlobalComputePDS(const ValueType& min, const ValueType& max)
{
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO("Trying type (PartitionedDataSet): " << svtkm::testing::TypeName<ValueType>::Name());

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

  svtkm::cont::ArrayHandle<svtkm::Range> ranges = svtkm::cont::FieldRangeGlobalCompute(mb, "pointvar");
  Validate(ranges, min, max);
}

static void TestFieldRangeGlobalCompute()
{
  svtkmdiy::mpi::communicator comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  PRINT_INFO_0("Running on " << comm.size() << " ranks.");

  // init random seed.
  std::srand(static_cast<unsigned int>(100 + 1024 * comm.rank()));

  TryRangeGlobalComputeDS<svtkm::Float64>(0, 1000);
  TryRangeGlobalComputeDS<svtkm::Int32>(-1024, 1024);
  TryRangeGlobalComputeDS<svtkm::Vec3f_32>(svtkm::make_Vec(1024, 0, -1024),
                                          svtkm::make_Vec(2048, 2048, 2048));
  TryRangeGlobalComputePDS<svtkm::Float64>(0, 1000);
  TryRangeGlobalComputePDS<svtkm::Int32>(-1024, 1024);
  TryRangeGlobalComputePDS<svtkm::Vec3f_32>(svtkm::make_Vec(1024, 0, -1024),
                                           svtkm::make_Vec(2048, 2048, 2048));
};
}

int UnitTestFieldRangeGlobalCompute(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestFieldRangeGlobalCompute, argc, argv);
}
