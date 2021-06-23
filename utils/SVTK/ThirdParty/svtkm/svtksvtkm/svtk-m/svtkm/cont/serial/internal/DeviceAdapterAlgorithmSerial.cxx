//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>

namespace svtkm
{
namespace cont
{

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>::ScheduleTask(
  svtkm::exec::serial::internal::TaskTiling1D& functor,
  svtkm::Id size)
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

  const svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  const svtkm::Id iterations = size / 1024;
  svtkm::Id index = 0;
  for (svtkm::Id i = 0; i < iterations; ++i)
  {
    functor(index, index + 1024);
    index += 1024;
  }
  functor(index, size);

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>::ScheduleTask(
  svtkm::exec::serial::internal::TaskTiling3D& functor,
  svtkm::Id3 size)
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

  const svtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  for (svtkm::Id k = 0; k < size[2]; ++k)
  {
    for (svtkm::Id j = 0; j < size[1]; ++j)
    {
      functor(0, size[0], j, k);
    }
  }

  if (errorMessage.IsErrorRaised())
  {
    throw svtkm::cont::ErrorExecution(errorString);
  }
}
}
}
