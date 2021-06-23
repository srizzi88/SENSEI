//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_examples_histogram_HistogramMPI_h
#define svtk_m_examples_histogram_HistogramMPI_h

#include <svtkm/filter/FilterField.h>

namespace example
{

/// \brief Construct the HistogramMPI of a given Field
///
/// Construct a HistogramMPI with a default of 10 bins.
///
class HistogramMPI : public svtkm::filter::FilterField<HistogramMPI>
{
public:
  //currently the HistogramMPI filter only works on scalar data.
  //this mainly has to do with getting the ranges for each bin
  //would require returning a more complex value type
  using SupportedTypes = svtkm::TypeListScalarAll;

  //Construct a HistogramMPI with a default of 10 bins
  SVTKM_CONT
  HistogramMPI();

  SVTKM_CONT
  void SetNumberOfBins(svtkm::Id count) { this->NumberOfBins = count; }

  SVTKM_CONT
  svtkm::Id GetNumberOfBins() const { return this->NumberOfBins; }

  //@{
  /// Get/Set the range to use to generate the HistogramMPI. If range is set to
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
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

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
} // namespace example

#include "HistogramMPI.hxx"

#endif // svtk_m_filter_Histogram_h
