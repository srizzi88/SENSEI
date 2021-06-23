//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractStructured_h
#define svtk_m_filter_ExtractStructured_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/ExtractStructured.h>

namespace svtkm
{
namespace filter
{
/// \brief Select piece (e.g., volume of interest) and/or subsample structured points dataset
///
/// Select or subsample a portion of an input structured dataset. The selected
/// portion of interested is referred to as the Volume Of Interest, or VOI.
/// The output of this filter is a structured dataset. The filter treats input
/// data of any topological dimension (i.e., point, line, plane, or volume) and
/// can generate output data of any topological dimension.
///
/// To use this filter set the VOI ivar which are i-j-k min/max indices that
/// specify a rectangular region in the data. (Note that these are 0-offset.)
/// You can also specify a sampling rate to subsample the data.
///
/// Typical applications of this filter are to extract a slice from a volume
/// for image processing, subsampling large volumes to reduce data size, or
/// extracting regions of a volume with interesting data.
///
class SVTKM_ALWAYS_EXPORT ExtractStructured : public svtkm::filter::FilterDataSet<ExtractStructured>
{
public:
  SVTKM_FILTER_EXPORT
  ExtractStructured();

  // Set the bounding box for the volume of interest
  SVTKM_CONT
  svtkm::RangeId3 GetVOI() const { return this->VOI; }

  SVTKM_CONT
  void SetVOI(svtkm::Id i0, svtkm::Id i1, svtkm::Id j0, svtkm::Id j1, svtkm::Id k0, svtkm::Id k1)
  {
    this->VOI = svtkm::RangeId3(i0, i1, j0, j1, k0, k1);
  }
  SVTKM_CONT
  void SetVOI(svtkm::Id extents[6]) { this->VOI = svtkm::RangeId3(extents); }
  SVTKM_CONT
  void SetVOI(svtkm::Id3 minPoint, svtkm::Id3 maxPoint)
  {
    this->VOI = svtkm::RangeId3(minPoint, maxPoint);
  }
  SVTKM_CONT
  void SetVOI(const svtkm::RangeId3& voi) { this->VOI = voi; }

  /// Get the Sampling rate
  SVTKM_CONT
  svtkm::Id3 GetSampleRate() const { return this->SampleRate; }

  /// Set the Sampling rate
  SVTKM_CONT
  void SetSampleRate(svtkm::Id i, svtkm::Id j, svtkm::Id k) { this->SampleRate = svtkm::Id3(i, j, k); }

  /// Set the Sampling rate
  SVTKM_CONT
  void SetSampleRate(svtkm::Id3 sampleRate) { this->SampleRate = sampleRate; }

  /// Get if we should include the outer boundary on a subsample
  SVTKM_CONT
  bool GetIncludeBoundary() { return this->IncludeBoundary; }
  /// Set if we should include the outer boundary on a subsample
  SVTKM_CONT
  void SetIncludeBoundary(bool value) { this->IncludeBoundary = value; }

  SVTKM_CONT
  void SetIncludeOffset(bool value) { this->IncludeOffset = value; }

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy>)
  {
    if (fieldMeta.IsPointField())
    {
      svtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessPointField(input);

      result.AddField(fieldMeta.AsField(output));
      return true;
    }

    // cell data must be scattered to the cells created per input cell
    if (fieldMeta.IsCellField())
    {
      svtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input);

      result.AddField(fieldMeta.AsField(output));
      return true;
    }

    return false;
  }

private:
  svtkm::RangeId3 VOI;
  svtkm::Id3 SampleRate = { 1, 1, 1 };
  bool IncludeBoundary;
  bool IncludeOffset;
  svtkm::worklet::ExtractStructured Worklet;
};

#ifndef svtkm_filter_ExtractStructured_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(ExtractStructured);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/ExtractStructured.hxx>

#endif // svtk_m_filter_ExtractStructured_h
