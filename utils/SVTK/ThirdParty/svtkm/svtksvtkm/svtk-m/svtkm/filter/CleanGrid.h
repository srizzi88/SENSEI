//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_CleanGrid_h
#define svtk_m_filter_CleanGrid_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/FilterDataSet.h>

#include <svtkm/worklet/PointMerge.h>
#include <svtkm/worklet/RemoveDegenerateCells.h>
#include <svtkm/worklet/RemoveUnusedPoints.h>

namespace svtkm
{
namespace filter
{

/// \brief Clean a mesh to an unstructured grid
///
/// This filter takes a data set and essentially copies it into a new data set.
/// The newly constructed data set will have the same cells as the input and
/// the topology will be stored in a \c CellSetExplicit<>. The filter will also
/// optionally remove all unused points.
///
/// Note that the result of \c CleanGrid is not necessarily smaller than the
/// input. For example, "cleaning" a data set with a \c CellSetStructured
/// topology will actually result in a much larger data set.
///
/// \todo Add a feature to merge points that are coincident or within a
/// tolerance.
///
class SVTKM_ALWAYS_EXPORT CleanGrid : public svtkm::filter::FilterDataSet<CleanGrid>
{
public:
  SVTKM_FILTER_EXPORT
  CleanGrid();

  /// When the CompactPointFields flag is true, the filter will identify any
  /// points that are not used by the topology. This is on by default.
  ///
  SVTKM_CONT bool GetCompactPointFields() const { return this->CompactPointFields; }
  SVTKM_CONT void SetCompactPointFields(bool flag) { this->CompactPointFields = flag; }

  /// When the MergePoints flag is true, the filter will identify any coincident
  /// points and merge them together. The distance two points can be to considered
  /// coincident is set with the tolerance flags. This is on by default.
  ///
  SVTKM_CONT bool GetMergePoints() const { return this->MergePoints; }
  SVTKM_CONT void SetMergePoints(bool flag) { this->MergePoints = flag; }

  /// Defines the tolerance used when determining whether two points are considered
  /// coincident. If the ToleranceIsAbsolute flag is false (the default), then this
  /// tolerance is scaled by the diagonal of the points.
  ///
  SVTKM_CONT svtkm::Float64 GetTolerance() const { return this->Tolerance; }
  SVTKM_CONT void SetTolerance(svtkm::Float64 tolerance) { this->Tolerance = tolerance; }

  /// When ToleranceIsAbsolute is false (the default) then the tolerance is scaled
  /// by the diagonal of the bounds of the dataset. If true, then the tolerance is
  /// taken as the actual distance to use.
  ///
  SVTKM_CONT bool GetToleranceIsAbsolute() const { return this->ToleranceIsAbsolute; }
  SVTKM_CONT void SetToleranceIsAbsolute(bool flag) { this->ToleranceIsAbsolute = flag; }

  /// Determine whether a cell is degenerate (that is, has repeated points that drops
  /// its dimensionalit) and removes them. This is on by default.
  ///
  SVTKM_CONT bool GetRemoveDegenerateCells() const { return this->RemoveDegenerateCells; }
  SVTKM_CONT void SetRemoveDegenerateCells(bool flag) { this->RemoveDegenerateCells = flag; }

  /// When FastMerge is true (the default), some corners are cut when computing
  /// coincident points. The point merge will go faster but the tolerance will not
  /// be strictly followed.
  ///
  SVTKM_CONT bool GetFastMerge() const { return this->FastMerge; }
  SVTKM_CONT void SetFastMerge(bool flag) { this->FastMerge = flag; }

  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& inData,
                                          svtkm::filter::PolicyBase<Policy> policy);


  template <typename ValueType, typename Storage, typename Policy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<ValueType, Storage>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<Policy>)
  {
    if (fieldMeta.IsPointField() && (this->GetCompactPointFields() || this->GetMergePoints()))
    {
      svtkm::cont::ArrayHandle<ValueType> compactedArray;
      if (this->GetCompactPointFields())
      {
        compactedArray = this->PointCompactor.MapPointFieldDeep(input);
        if (this->GetMergePoints())
        {
          compactedArray = this->PointMerger.MapPointField(compactedArray);
        }
      }
      else if (this->GetMergePoints())
      {
        compactedArray = this->PointMerger.MapPointField(input);
      }
      result.AddField(fieldMeta.AsField(compactedArray));
    }
    else if (fieldMeta.IsCellField() && this->GetRemoveDegenerateCells())
    {
      result.AddField(fieldMeta.AsField(this->CellCompactor.ProcessCellField(input)));
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
    }

    return true;
  }

private:
  bool CompactPointFields;
  bool MergePoints;
  svtkm::Float64 Tolerance;
  bool ToleranceIsAbsolute;
  bool RemoveDegenerateCells;
  bool FastMerge;

  SVTKM_FILTER_EXPORT svtkm::cont::DataSet GenerateOutput(
    const svtkm::cont::DataSet& inData,
    svtkm::cont::CellSetExplicit<>& outputCellSet);

  svtkm::worklet::RemoveUnusedPoints PointCompactor;
  svtkm::worklet::RemoveDegenerateCells CellCompactor;
  svtkm::worklet::PointMerge PointMerger;
};

#ifndef svtkm_filter_CleanGrid_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(CleanGrid);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/CleanGrid.hxx>

#endif //svtk_m_filter_CleanGrid_h
