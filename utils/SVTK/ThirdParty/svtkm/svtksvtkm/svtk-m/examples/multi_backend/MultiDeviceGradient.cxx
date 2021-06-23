//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtk_m_examples_multibackend_MultiDeviceGradient_cxx

#include "MultiDeviceGradient.h"
#include "MultiDeviceGradient.hxx"

template svtkm::cont::PartitionedDataSet MultiDeviceGradient::PrepareForExecution<
  svtkm::filter::PolicyDefault>(const svtkm::cont::PartitionedDataSet&,
                               const svtkm::filter::PolicyBase<svtkm::filter::PolicyDefault>&);
