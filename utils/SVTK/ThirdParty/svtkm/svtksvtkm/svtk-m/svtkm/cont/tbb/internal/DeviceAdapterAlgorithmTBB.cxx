//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>

namespace svtkm
{
namespace cont
{

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTBB>::ScheduleTask(
  svtkm::exec::tbb::internal::TaskTiling1D& functor,
  svtkm::Id size)
{
  const svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  ::tbb::blocked_range<svtkm::Id> range(0, size, tbb::TBB_GRAIN_SIZE);

  ::tbb::parallel_for(
    range, [&](const ::tbb::blocked_range<svtkm::Id>& r) { functor(r.begin(), r.end()); });

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTBB>::ScheduleTask(
  svtkm::exec::tbb::internal::TaskTiling3D& functor,
  svtkm::Id3 size)
{
  static constexpr svtkm::UInt32 TBB_GRAIN_SIZE_3D[3] = { 1, 4, 256 };
  const svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  //memory is generally setup in a way that iterating the first range
  //in the tightest loop has the best cache coherence.
  ::tbb::blocked_range3d<svtkm::Id> range(0,
                                         size[2],
                                         TBB_GRAIN_SIZE_3D[0],
                                         0,
                                         size[1],
                                         TBB_GRAIN_SIZE_3D[1],
                                         0,
                                         size[0],
                                         TBB_GRAIN_SIZE_3D[2]);
  ::tbb::parallel_for(range, [&](const ::tbb::blocked_range3d<svtkm::Id>& r) {
    for (svtkm::Id k = r.pages().begin(); k != r.pages().end(); ++k)
    {
      for (svtkm::Id j = r.rows().begin(); j != r.rows().end(); ++j)
      {
        const svtkm::Id start = r.cols().begin();
        const svtkm::Id end = r.cols().end();
        functor(start, end, j, k);
      }
    }
  });

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}
}
}
