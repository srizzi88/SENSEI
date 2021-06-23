//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExternalFaces_h
#define svtk_m_filter_ExternalFaces_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/ExternalFaces.h>

namespace svtkm
{
namespace filter
{

/// \brief  Extract external faces of a geometry
///
/// ExternalFaces is a filter that extracts all external faces from a
/// data set. An external face is defined is defined as a face/side of a cell
/// that belongs only to one cell in the entire mesh.
/// @warning
/// This filter is currently only supports propagation of point properties
///
class SVTKM_ALWAYS_EXPORT ExternalFaces : public svtkm::filter::FilterDataSet<ExternalFaces>
{
public:
  SVTKM_FILTER_EXPORT
  ExternalFaces();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the
  // unused elements
  SVTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  SVTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // When PassPolyData is set (the default), incoming poly data (0D, 1D, and 2D cells)
  // will be passed to the output external faces data set.
  SVTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }
  SVTKM_CONT
  void SetPassPolyData(bool value)
  {
    this->PassPolyData = value;
    this->Worklet.SetPassPolyData(value);
  }

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    if (fieldMeta.IsPointField())
    {
      if (this->CompactPoints)
      {
        return this->Compactor.DoMapField(result, input, fieldMeta, policy);
      }
      else
      {
        result.AddField(fieldMeta.AsField(input));
        return true;
      }
    }
    else if (fieldMeta.IsCellField())
    {
      svtkm::cont::ArrayHandle<T> fieldArray;
      fieldArray = this->Worklet.ProcessCellField(input);
      result.AddField(fieldMeta.AsField(fieldArray));
      return true;
    }

    return false;
  }

private:
  bool CompactPoints;
  bool PassPolyData;

  SVTKM_FILTER_EXPORT svtkm::cont::DataSet GenerateOutput(const svtkm::cont::DataSet& input,
                                                        svtkm::cont::CellSetExplicit<>& outCellSet);

  svtkm::filter::CleanGrid Compactor;
  svtkm::worklet::ExternalFaces Worklet;
};
#ifndef svtkm_filter_ExternalFaces_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(ExternalFaces);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/ExternalFaces.hxx>

#endif // svtk_m_filter_ExternalFaces_h
