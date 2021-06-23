//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_examples_multibackend_MultiDeviceGradient_h
#define svtk_m_examples_multibackend_MultiDeviceGradient_h

#include <svtkm/filter/FilterField.h>

#include "TaskQueue.h"

#include <thread>

using RuntimeTaskQueue = TaskQueue<std::function<void()>>;

/// \brief Construct a MultiDeviceGradient for a given partitioned dataset
///
/// The Policy used with MultiDeviceGradient must include the TBB and CUDA
/// backends.
class MultiDeviceGradient : public svtkm::filter::FilterField<MultiDeviceGradient>
{
public:
  using SupportedTypes = svtkm::List<svtkm::Float32, svtkm::Float64, svtkm::Vec3f_32, svtkm::Vec3f_64>;

  //Construct a MultiDeviceGradient and worker pool
  SVTKM_CONT
  MultiDeviceGradient();

  //Needed so that we can shut down the worker pool properly
  SVTKM_CONT
  ~MultiDeviceGradient();

  /// When this flag is on (default is off), the gradient filter will provide a
  /// point based gradients, which are significantly more costly since for each
  /// point we need to compute the gradient of each cell that uses it.
  void SetComputePointGradient(bool enable) { ComputePointGradient = enable; }
  bool GetComputePointGradient() const { return ComputePointGradient; }

  /// Will submit each block to a work queue that the threads will
  /// pull work from
  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::PartitionedDataSet PrepareForExecution(
    const svtkm::cont::PartitionedDataSet&,
    const svtkm::filter::PolicyBase<DerivedPolicy>&);

private:
  bool ComputePointGradient;
  RuntimeTaskQueue Queue;
  std::vector<std::thread> Workers;
};

#ifndef svtk_m_examples_multibackend_MultiDeviceGradient_cxx
extern template svtkm::cont::PartitionedDataSet MultiDeviceGradient::PrepareForExecution<
  svtkm::filter::PolicyDefault>(const svtkm::cont::PartitionedDataSet&,
                               const svtkm::filter::PolicyBase<svtkm::filter::PolicyDefault>&);
#endif

#endif
