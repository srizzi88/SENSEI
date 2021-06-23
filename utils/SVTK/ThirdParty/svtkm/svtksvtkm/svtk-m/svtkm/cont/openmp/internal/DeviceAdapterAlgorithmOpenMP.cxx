//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <svtkm/cont/openmp/internal/FunctorsOpenMP.h>

#include <svtkm/cont/ErrorExecution.h>

#include <omp.h>

namespace svtkm
{
namespace cont
{

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagOpenMP>::ScheduleTask(
  svtkm::exec::openmp::internal::TaskTiling1D& functor,
  svtkm::Id size)
{
  static constexpr svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  static constexpr svtkm::Id CHUNK_SIZE = 1024;

  SVTKM_OPENMP_DIRECTIVE(parallel for
                        schedule(guided))
  for (svtkm::Id i = 0; i < size; i += CHUNK_SIZE)
  {
    const svtkm::Id end = std::min(i + CHUNK_SIZE, size);
    functor(i, end);
  }

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagOpenMP>::ScheduleTask(
  svtkm::exec::openmp::internal::TaskTiling3D& functor,
  svtkm::Id3 size)
{
  static constexpr svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  svtkm::Id3 chunkDims;
  if (size[0] > 512)
  {
    chunkDims = { 1024, 4, 1 };
  }
  else if (size[0] > 256)
  {
    chunkDims = { 512, 4, 2 };
  }
  else if (size[0] > 128)
  {
    chunkDims = { 256, 4, 4 };
  }
  else if (size[0] > 64)
  {
    chunkDims = { 128, 8, 4 };
  }
  else if (size[0] > 32)
  {
    chunkDims = { 64, 8, 8 };
  }
  else if (size[0] > 16)
  {
    chunkDims = { 32, 16, 8 };
  }
  else
  {
    chunkDims = { 16, 16, 16 };
  }

  const svtkm::Id3 numChunks{ openmp::CeilDivide(size[0], chunkDims[0]),
                             openmp::CeilDivide(size[1], chunkDims[1]),
                             openmp::CeilDivide(size[2], chunkDims[2]) };
  const svtkm::Id chunkCount = numChunks[0] * numChunks[1] * numChunks[2];

  // Lambda to convert chunkIdx into a start/end {i, j, k}:
  auto computeIJK = [&](const svtkm::Id& chunkIdx, svtkm::Id3& start, svtkm::Id3& end) {
    start[0] = chunkIdx % numChunks[0];
    start[1] = (chunkIdx / numChunks[0]) % numChunks[1];
    start[2] = (chunkIdx / (numChunks[0] * numChunks[1]));
    start *= chunkDims; // c-wise mult

    end[0] = std::min(start[0] + chunkDims[0], size[0]);
    end[1] = std::min(start[1] + chunkDims[1], size[1]);
    end[2] = std::min(start[2] + chunkDims[2], size[2]);
  };

  // Iterate through each chunk, converting the chunkIdx into an ijk range:
  SVTKM_OPENMP_DIRECTIVE(parallel for
                        schedule(guided))
  for (svtkm::Id chunkIdx = 0; chunkIdx < chunkCount; ++chunkIdx)
  {
    svtkm::Id3 startIJK;
    svtkm::Id3 endIJK;
    computeIJK(chunkIdx, startIJK, endIJK);

    for (svtkm::Id k = startIJK[2]; k < endIJK[2]; ++k)
    {
      for (svtkm::Id j = startIJK[1]; j < endIJK[1]; ++j)
      {
        functor(startIJK[0], endIJK[0], j, k);
      }
    }
  }

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}
}
} // end namespace svtkm::cont
