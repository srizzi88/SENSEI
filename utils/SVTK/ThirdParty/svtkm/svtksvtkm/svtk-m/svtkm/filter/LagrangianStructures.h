//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_LagrangianStructures_h
#define svtk_m_filter_LagrangianStructures_h

#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>

#include <svtkm/filter/FilterDataSetWithField.h>

#include <svtkm/worklet/ParticleAdvection.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/Integrators.h>

namespace svtkm
{
namespace filter
{

class LagrangianStructures : public svtkm::filter::FilterDataSetWithField<LagrangianStructures>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  LagrangianStructures();

  void SetStepSize(svtkm::FloatDefault s) { this->StepSize = s; }
  svtkm::FloatDefault GetStepSize() { return this->StepSize; }

  void SetNumberOfSteps(svtkm::Id n) { this->NumberOfSteps = n; }
  svtkm::Id GetNumberOfSteps() { return this->NumberOfSteps; }

  void SetAdvectionTime(svtkm::FloatDefault advectionTime) { this->AdvectionTime = advectionTime; }
  svtkm::FloatDefault GetAdvectionTime() { return this->AdvectionTime; }

  void SetUseAuxiliaryGrid(bool useAuxiliaryGrid) { this->UseAuxiliaryGrid = useAuxiliaryGrid; }
  bool GetUseAuxiliaryGrid() { return this->UseAuxiliaryGrid; }

  void SetAuxiliaryGridDimensions(svtkm::Id3 auxiliaryDims) { this->AuxiliaryDims = auxiliaryDims; }
  svtkm::Id3 GetAuxiliaryGridDimensions() { return this->AuxiliaryDims; }

  void SetUseFlowMapOutput(bool useFlowMapOutput) { this->UseFlowMapOutput = useFlowMapOutput; }
  bool GetUseFlowMapOutput() { return this->UseFlowMapOutput; }

  void SetOutputFieldName(std::string outputFieldName) { this->OutputFieldName = outputFieldName; }
  std::string GetOutputFieldName() { return this->OutputFieldName; }

  inline void SetFlowMapOutput(svtkm::cont::ArrayHandle<svtkm::Vec3f>& flowMap)
  {
    this->FlowMapOutput = flowMap;
  }
  inline svtkm::cont::ArrayHandle<svtkm::Vec3f> GetFlowMapOutput() { return this->FlowMapOutput; }

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
  svtkm::FloatDefault StepSize;
  svtkm::Id NumberOfSteps;
  svtkm::FloatDefault AdvectionTime;
  bool UseAuxiliaryGrid = false;
  svtkm::Id3 AuxiliaryDims;
  bool UseFlowMapOutput = false;
  std::string OutputFieldName;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> FlowMapOutput;
};

} // namespace filter
} // namespace svtkm

#include <svtkm/filter/LagrangianStructures.hxx>

#endif // svtk_m_filter_LagrangianStructures_h
