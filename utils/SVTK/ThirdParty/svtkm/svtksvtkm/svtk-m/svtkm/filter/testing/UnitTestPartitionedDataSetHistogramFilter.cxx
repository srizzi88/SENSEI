//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/filter/Histogram.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/PartitionedDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

namespace
{

static unsigned int uid = 1;

template <typename T>
svtkm::cont::ArrayHandle<T> CreateArrayHandle(T min, T max, svtkm::Id numVals)
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

template <typename T, int size>
svtkm::cont::ArrayHandle<svtkm::Vec<T, size>> CreateArrayHandle(const svtkm::Vec<T, size>& min,
                                                              const svtkm::Vec<T, size>& max,
                                                              svtkm::Id numVals)
{
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
                  svtkm::Vec<T, size> val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<T>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}


template <typename T>
void AddField(svtkm::cont::DataSet& dataset,
              const T& min,
              const T& max,
              svtkm::Id numVals,
              const std::string& name,
              svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::POINTS)
{
  auto ah = CreateArrayHandle(min, max, numVals);
  dataset.AddField(svtkm::cont::Field(name, assoc, ah));
}
}

static void TestPartitionedDataSetHistogram()
{
  // init random seed.
  std::srand(100);

  svtkm::cont::PartitionedDataSet mb;

  svtkm::cont::DataSet partition0;
  AddField<double>(partition0, 0.0, 100.0, 1024, "double");
  mb.AppendPartition(partition0);

  svtkm::cont::DataSet partition1;
  AddField<int>(partition1, 100, 1000, 1024, "double");
  mb.AppendPartition(partition1);

  svtkm::cont::DataSet partition2;
  AddField<double>(partition2, 100.0, 500.0, 1024, "double");
  mb.AppendPartition(partition2);

  svtkm::filter::Histogram histogram;
  histogram.SetActiveField("double");
  auto result = histogram.Execute(mb);
  SVTKM_TEST_ASSERT(result.GetNumberOfPartitions() == 1, "Expecting 1 partition.");

  auto bins = result.GetPartition(0)
                .GetField("histogram")
                .GetData()
                .Cast<svtkm::cont::ArrayHandle<svtkm::Id>>();
  SVTKM_TEST_ASSERT(bins.GetNumberOfValues() == 10, "Expecting 10 bins.");
  auto count = std::accumulate(svtkm::cont::ArrayPortalToIteratorBegin(bins.GetPortalConstControl()),
                               svtkm::cont::ArrayPortalToIteratorEnd(bins.GetPortalConstControl()),
                               svtkm::Id(0),
                               svtkm::Add());
  SVTKM_TEST_ASSERT(count == 1024 * 3, "Expecting 3072 values");

  std::cout << "Values [" << count << "] =";
  for (int cc = 0; cc < 10; ++cc)
  {
    std::cout << " " << bins.GetPortalConstControl().Get(cc);
  }
  std::cout << std::endl;
};

int UnitTestPartitionedDataSetHistogramFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestPartitionedDataSetHistogram, argc, argv);
}
