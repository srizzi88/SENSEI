//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ScatterCounting_h
#define svtk_m_worklet_ScatterCounting_h

#include <svtkm/worklet/internal/ScatterBase.h>
#include <svtkm/worklet/svtkm_worklet_export.h>

#include <svtkm/cont/VariantArrayHandle.h>

#include <sstream>

namespace svtkm
{
namespace worklet
{

namespace detail
{

struct ScatterCountingBuilder;

} // namespace detail

/// \brief A scatter that maps input to some numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterCounting establishes a 1 to
/// N mapping from input to output. That is, every input element generates 0 or
/// more output elements associated with it. The output elements are grouped by
/// the input associated.
///
/// A counting scatter takes an array of counts for each input. The data is
/// taken in the constructor and the index arrays are derived from that. So
/// changing the counts after the scatter is created will have no effect.
///
struct SVTKM_WORKLET_EXPORT ScatterCounting : internal::ScatterBase
{
  using CountTypes = svtkm::List<svtkm::Int64,
                                svtkm::Int32,
                                svtkm::Int16,
                                svtkm::Int8,
                                svtkm::UInt64,
                                svtkm::UInt32,
                                svtkm::UInt16,
                                svtkm::UInt8>;
  using VariantArrayHandleCount = svtkm::cont::VariantArrayHandleBase<CountTypes>;

  /// Construct a \c ScatterCounting object using an array of counts for the
  /// number of outputs for each input. Part of the construction requires
  /// generating an input to output map, but this map is not needed for the
  /// operations of \c ScatterCounting, so by default it is deleted. However,
  /// other users might make use of it, so you can instruct the constructor
  /// to save the input to output map.
  ///
  template <typename TypeList>
  SVTKM_CONT ScatterCounting(const svtkm::cont::VariantArrayHandleBase<TypeList>& countArray,
                            svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny(),
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(VariantArrayHandleCount(countArray), device, saveInputToOutputMap);
  }
  SVTKM_CONT ScatterCounting(const VariantArrayHandleCount& countArray,
                            svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny(),
                            bool saveInputToOutputMap = false)
  {
    this->BuildArrays(countArray, device, saveInputToOutputMap);
  }
  template <typename TypeList>
  SVTKM_CONT ScatterCounting(const svtkm::cont::VariantArrayHandleBase<TypeList>& countArray,
                            bool saveInputToOutputMap)
  {
    this->BuildArrays(
      VariantArrayHandleCount(countArray), svtkm::cont::DeviceAdapterTagAny(), saveInputToOutputMap);
  }
  SVTKM_CONT ScatterCounting(const VariantArrayHandleCount& countArray, bool saveInputToOutputMap)
  {
    this->BuildArrays(countArray, svtkm::cont::DeviceAdapterTagAny(), saveInputToOutputMap);
  }

  using OutputToInputMapType = svtkm::cont::ArrayHandle<svtkm::Id>;

  template <typename RangeType>
  SVTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->OutputToInputMap;
  }

  using VisitArrayType = svtkm::cont::ArrayHandle<svtkm::IdComponent>;
  template <typename RangeType>
  SVTKM_CONT VisitArrayType GetVisitArray(RangeType) const
  {
    return this->VisitArray;
  }

  SVTKM_CONT
  svtkm::Id GetOutputRange(svtkm::Id inputRange) const
  {
    if (inputRange != this->InputRange)
    {
      std::stringstream msg;
      msg << "ScatterCounting initialized with input domain of size " << this->InputRange
          << " but used with a worklet invoke of size " << inputRange << std::endl;
      throw svtkm::cont::ErrorBadValue(msg.str());
    }
    return this->VisitArray.GetNumberOfValues();
  }
  SVTKM_CONT
  svtkm::Id GetOutputRange(svtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  SVTKM_CONT
  OutputToInputMapType GetOutputToInputMap() const { return this->OutputToInputMap; }

  /// This array will not be valid unless explicitly instructed to be saved.
  /// (See documentation for the constructor.)
  ///
  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::Id> GetInputToOutputMap() const { return this->InputToOutputMap; }

private:
  svtkm::Id InputRange;
  svtkm::cont::ArrayHandle<svtkm::Id> InputToOutputMap;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;

  friend struct detail::ScatterCountingBuilder;

  SVTKM_CONT void BuildArrays(const VariantArrayHandleCount& countArray,
                             svtkm::cont::DeviceAdapterId device,
                             bool saveInputToOutputMap);
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_ScatterCounting_h
