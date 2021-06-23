//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_MaskSelect_h
#define svtk_m_worklet_MaskSelect_h

#include <svtkm/worklet/internal/MaskBase.h>
#include <svtkm/worklet/svtkm_worklet_export.h>

#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace worklet
{

/// \brief Mask using arrays to select specific elements to suppress.
///
/// \c MaskSelect is a worklet mask object that is used to select elements in the output of a
/// worklet to suppress the invocation. That is, the worklet will only be invoked for elements in
/// the output that are not masked out by the given array.
///
/// \c MaskSelect is initialized with a mask array. This array should contain a 0 for any entry
/// that should be masked and a 1 for any output that should be generated. It is an error to have
/// any value that is not a 0 or 1. This method is slower than specifying an index array.
///
class SVTKM_WORKLET_EXPORT MaskSelect : public internal::MaskBase
{
  using MaskTypes =
    svtkm::List<svtkm::Int32, svtkm::Int64, svtkm::UInt32, svtkm::UInt64, svtkm::Int8, svtkm::UInt8, char>;
  using VariantArrayHandleMask = svtkm::cont::VariantArrayHandleBase<MaskTypes>;

public:
  using ThreadToOutputMapType = svtkm::cont::ArrayHandle<svtkm::Id>;

  MaskSelect(const VariantArrayHandleMask& maskArray,
             svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(maskArray, device);
  }

  template <typename TypeList>
  MaskSelect(const svtkm::cont::VariantArrayHandleBase<TypeList>& indexArray,
             svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
  {
    this->ThreadToOutputMap = this->Build(VariantArrayHandleMask(indexArray), device);
  }

  template <typename RangeType>
  svtkm::Id GetThreadRange(RangeType svtkmNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap.GetNumberOfValues();
  }

  template <typename RangeType>
  ThreadToOutputMapType GetThreadToOutputMap(RangeType svtkmNotUsed(outputRange)) const
  {
    return this->ThreadToOutputMap;
  }

private:
  ThreadToOutputMapType ThreadToOutputMap;

  SVTKM_CONT ThreadToOutputMapType Build(const VariantArrayHandleMask& maskArray,
                                        svtkm::cont::DeviceAdapterId device);
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_MaskSelect_h
