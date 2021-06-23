//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Streamline_h
#define svtk_m_filter_Streamline_h

#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/worklet/ParticleAdvection.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/Integrators.h>

namespace svtkm
{
namespace filter
{
/// \brief generate streamlines from a vector field.

/// Takes as input a vector field and seed locations and generates the
/// paths taken by the seeds through the vector field.
class Streamline : public svtkm::filter::FilterDataSetWithField<Streamline>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  Streamline();

  SVTKM_CONT
  void SetStepSize(svtkm::FloatDefault s) { this->StepSize = s; }

  SVTKM_CONT
  void SetNumberOfSteps(svtkm::Id n) { this->NumberOfSteps = n; }

  SVTKM_CONT
  void SetSeeds(svtkm::cont::ArrayHandle<svtkm::Particle>& seeds);

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::Id NumberOfSteps;
  svtkm::FloatDefault StepSize;
  svtkm::cont::ArrayHandle<svtkm::Particle> Seeds;
  svtkm::worklet::Streamline Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Streamline.hxx>

#endif // svtk_m_filter_Streamline_h
