//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Tube_h
#define svtk_m_filter_Tube_h

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Tube.h>

namespace svtkm
{
namespace filter
{
/// \brief generate tube geometry from polylines.

/// Takes as input a set of polylines, radius, num sides and capping flag.
/// Produces tubes along each polyline

class Tube : public svtkm::filter::FilterDataSet<Tube>
{
public:
  SVTKM_CONT
  Tube();

  SVTKM_CONT
  void SetRadius(svtkm::FloatDefault r) { this->Radius = r; }

  SVTKM_CONT
  void SetNumberOfSides(svtkm::Id n) { this->NumberOfSides = n; }

  SVTKM_CONT
  void SetCapping(bool v) { this->Capping = v; }

  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<Policy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename Policy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<Policy> policy);

private:
  svtkm::worklet::Tube Worklet;
  svtkm::FloatDefault Radius;
  svtkm::Id NumberOfSides;
  bool Capping;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Tube.hxx>

#endif // svtk_m_filter_Tube_h
