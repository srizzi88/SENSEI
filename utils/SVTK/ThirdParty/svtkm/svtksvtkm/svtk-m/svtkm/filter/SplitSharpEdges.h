//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_SplitSharpEdges_h
#define svtk_m_filter_SplitSharpEdges_h

#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/worklet/SplitSharpEdges.h>

namespace svtkm
{
namespace filter
{

/// \brief Split sharp manifold edges where the feature angle between the
///  adjacent surfaces are larger than the threshold value
///
/// Split sharp manifold edges where the feature angle between the adjacent
/// surfaces are larger than the threshold value. When an edge is split, it
/// would add a new point to the coordinates and update the connectivity of
/// an adjacent surface.
/// Ex. there are two adjacent triangles(0,1,2) and (2,1,3). Edge (1,2) needs
/// to be split. Two new points 4(duplication of point 1) an 5(duplication of point 2)
/// would be added and the later triangle's connectivity would be changed
/// to (5,4,3).
/// By default, all old point's fields would be copied to the new point.
/// Use with caution.
class SplitSharpEdges : public svtkm::filter::FilterDataSetWithField<SplitSharpEdges>
{
public:
  // SplitSharpEdges filter needs cell normals to decide split.
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  SplitSharpEdges();

  SVTKM_CONT
  void SetFeatureAngle(svtkm::FloatDefault value) { this->FeatureAngle = value; }

  SVTKM_CONT
  svtkm::FloatDefault GetFeatureAngle() const { return this->FeatureAngle; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::FloatDefault FeatureAngle;
  svtkm::worklet::SplitSharpEdges Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/SplitSharpEdges.hxx>

#endif // svtk_m_filter_SplitSharpEdges_h
