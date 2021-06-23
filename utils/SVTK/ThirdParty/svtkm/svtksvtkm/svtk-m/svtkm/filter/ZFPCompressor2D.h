//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ZFPCompressor2D_h
#define svtk_m_filter_ZFPCompressor2D_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/ZFP2DCompressor.h>

namespace svtkm
{
namespace filter
{
/// \brief Compress a scalar field using ZFP

/// Takes as input a 1D array and generates on
/// output of compressed data.
/// @warning
/// This filter is currently only supports 1D volumes.
class ZFPCompressor2D : public svtkm::filter::FilterField<ZFPCompressor2D>
{
public:
  using SupportedTypes = svtkm::List<svtkm::Int32, svtkm::Float32, svtkm::Float64>;

  SVTKM_CONT
  ZFPCompressor2D();

  void SetRate(svtkm::Float64 _rate) { rate = _rate; }
  svtkm::Float64 GetRate() { return rate; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  svtkm::Float64 rate;
  svtkm::worklet::ZFP2DCompressor compressor;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ZFPCompressor2D.hxx>

#endif // svtk_m_filter_ZFPCompressor2D_h
