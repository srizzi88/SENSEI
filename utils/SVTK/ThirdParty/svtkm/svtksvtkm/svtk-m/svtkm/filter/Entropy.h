//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Entropy_h
#define svtk_m_filter_Entropy_h

#include <svtkm/filter/FilterField.h>

namespace svtkm
{
namespace filter
{

/// \brief Construct the entropy histogram of a given Field
///
/// Construct a histogram which is used to compute the entropy with a default of 10 bins
///
class Entropy : public svtkm::filter::FilterField<Entropy>
{
public:
  //currently the Entropy filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  //Construct a histogram which is used to compute the entropy with a default of 10 bins
  SVTKM_CONT
  Entropy();

  SVTKM_CONT
  void SetNumberOfBins(svtkm::Id count) { this->NumberOfBins = count; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  svtkm::Id NumberOfBins;
};
}
} // namespace svtkm::filter


#include <svtkm/filter/Entropy.hxx>

#endif // svtk_m_filter_Entropy_h
