//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Histogram_h
#define svtk_m_filter_Histogram_h

#include <svtkm/filter/FilterField.h>

namespace svtkm
{
namespace filter
{

/// \brief Construct the histogram of a given Field
///
/// Construct a histogram with a default of 10 bins.
///
class Histogram : public svtkm::filter::FilterField<Histogram>
{
public:
  using SupportedTypes = svtkm::TypeListScalarAll;

  //Construct a histogram with a default of 10 bins
  SVTKM_CONT
  Histogram();

  SVTKM_CONT
  void SetNumberOfBins(svtkm::Id count) { this->NumberOfBins = count; }

  SVTKM_CONT
  svtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

  //@{
  /// Get/Set the range to use to generate the histogram. If range is set to
  /// empty, the field's global range (computed using `svtkm::cont::FieldRangeGlobalCompute`)
  /// will be used.
  SVTKM_CONT
  void SetRange(const svtkm::Range& range) { this->Range = range; }

  SVTKM_CONT
  const svtkm::Range& GetRange() const { return this->Range; }
  //@}

  /// Returns the bin delta of the last computed field.
  SVTKM_CONT
  svtkm::Float64 GetBinDelta() const { return this->BinDelta; }

  /// Returns the range used for most recent execute. If `SetRange` is used to
  /// specify and non-empty range, then this will be same as the range after
  /// the `Execute` call.
  SVTKM_CONT
  svtkm::Range GetComputedRange() const { return this->ComputedRange; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //@{
  /// when operating on svtkm::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  template <typename DerivedPolicy>
  SVTKM_CONT void PreExecute(const svtkm::cont::PartitionedDataSet& input,
                            const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  SVTKM_CONT void PostExecute(const svtkm::cont::PartitionedDataSet& input,
                             svtkm::cont::PartitionedDataSet& output,
                             const svtkm::filter::PolicyBase<DerivedPolicy>&);
  //@}

private:
  svtkm::Id NumberOfBins;
  svtkm::Float64 BinDelta;
  svtkm::Range ComputedRange;
  svtkm::Range Range;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Histogram.hxx>

#endif // svtk_m_filter_Histogram_h
