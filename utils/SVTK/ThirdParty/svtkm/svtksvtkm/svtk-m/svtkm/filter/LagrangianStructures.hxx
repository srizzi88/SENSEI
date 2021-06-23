//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_LagrangianStructures_hxx
#define svtk_m_filter_LagrangianStructures_hxx

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/Integrators.h>
#include <svtkm/worklet/particleadvection/Particles.h>

#include <svtkm/worklet/LagrangianStructures.h>

namespace svtkm
{
namespace filter
{

namespace detail
{
class ExtractParticlePosition : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, FieldOut position);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  SVTKM_EXEC void operator()(const svtkm::Particle& particle, svtkm::Vec3f& pt) const
  {
    pt = particle.Pos;
  }
};

} //detail

//-----------------------------------------------------------------------------
inline SVTKM_CONT LagrangianStructures::LagrangianStructures()
  : svtkm::filter::FilterDataSetWithField<LagrangianStructures>()
{
  OutputFieldName = std::string("FTLE");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet LagrangianStructures::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (!fieldMeta.IsPointField())
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  using Structured2DType = svtkm::cont::CellSetStructured<2>;
  using Structured3DType = svtkm::cont::CellSetStructured<3>;

  using FieldHandle = svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>;
  using GridEvaluator = svtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using Integrator = svtkm::worklet::particleadvection::RK4Integrator<GridEvaluator>;

  svtkm::FloatDefault stepSize = this->GetStepSize();
  svtkm::Id numberOfSteps = this->GetNumberOfSteps();

  svtkm::cont::CoordinateSystem coordinates = input.GetCoordinateSystem();
  svtkm::cont::DynamicCellSet cellset = input.GetCellSet();

  svtkm::cont::DataSet lcsInput;
  if (this->GetUseAuxiliaryGrid())
  {
    svtkm::Id3 lcsGridDims = this->GetAuxiliaryGridDimensions();
    svtkm::Bounds inputBounds = coordinates.GetBounds();
    svtkm::Vec3f origin(static_cast<svtkm::FloatDefault>(inputBounds.X.Min),
                       static_cast<svtkm::FloatDefault>(inputBounds.Y.Min),
                       static_cast<svtkm::FloatDefault>(inputBounds.Z.Min));
    svtkm::Vec3f spacing;
    spacing[0] = static_cast<svtkm::FloatDefault>(inputBounds.X.Length()) /
      static_cast<svtkm::FloatDefault>(lcsGridDims[0] - 1);
    spacing[1] = static_cast<svtkm::FloatDefault>(inputBounds.Y.Length()) /
      static_cast<svtkm::FloatDefault>(lcsGridDims[1] - 1);
    spacing[2] = static_cast<svtkm::FloatDefault>(inputBounds.Z.Length()) /
      static_cast<svtkm::FloatDefault>(lcsGridDims[2] - 1);
    svtkm::cont::DataSetBuilderUniform uniformDatasetBuilder;
    lcsInput = uniformDatasetBuilder.Create(lcsGridDims, origin, spacing);
  }
  else
  {
    // Check if input dataset is structured.
    // If not, we cannot proceed.
    if (!(cellset.IsType<Structured2DType>() || cellset.IsType<Structured3DType>()))
      throw svtkm::cont::ErrorFilterExecution(
        "Provided data is not structured, provide parameters for an auxiliary grid.");
    lcsInput = input;
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f> lcsInputPoints, lcsOutputPoints;
  svtkm::cont::ArrayCopy(lcsInput.GetCoordinateSystem().GetData(), lcsInputPoints);
  if (this->GetUseFlowMapOutput())
  {
    // Check if there is a 1:1 correspondense between the flow map
    // and the input points
    lcsOutputPoints = this->GetFlowMapOutput();
    if (lcsInputPoints.GetNumberOfValues() != lcsOutputPoints.GetNumberOfValues())
      throw svtkm::cont::ErrorFilterExecution(
        "Provided flow map does not correspond to the input points for LCS filter.");
  }
  else
  {
    GridEvaluator evaluator(input.GetCoordinateSystem(), input.GetCellSet(), field);
    Integrator integrator(evaluator, stepSize);
    svtkm::worklet::ParticleAdvection particles;
    svtkm::worklet::ParticleAdvectionResult advectionResult;
    svtkm::cont::ArrayHandle<svtkm::Vec3f> advectionPoints;
    svtkm::cont::ArrayCopy(lcsInputPoints, advectionPoints);
    advectionResult = particles.Run(integrator, advectionPoints, numberOfSteps);

    svtkm::cont::Invoker invoke;
    invoke(detail::ExtractParticlePosition{}, advectionResult.Particles, lcsOutputPoints);
  }
  // FTLE output field
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> outputField;
  svtkm::FloatDefault advectionTime = this->GetAdvectionTime();

  svtkm::cont::DynamicCellSet lcsCellSet = lcsInput.GetCellSet();
  if (lcsCellSet.IsType<Structured2DType>())
  {
    using AnalysisType = svtkm::worklet::LagrangianStructures<2>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    svtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }
  else if (lcsCellSet.IsType<Structured3DType>())
  {
    using AnalysisType = svtkm::worklet::LagrangianStructures<3>;
    AnalysisType ftleCalculator(advectionTime, lcsCellSet);
    svtkm::worklet::DispatcherMapField<AnalysisType> dispatcher(ftleCalculator);
    dispatcher.Invoke(lcsInputPoints, lcsOutputPoints, outputField);
  }

  svtkm::cont::DataSet output;
  svtkm::cont::DataSetFieldAdd fieldAdder;
  output.AddCoordinateSystem(lcsInput.GetCoordinateSystem());
  output.SetCellSet(lcsInput.GetCellSet());
  fieldAdder.AddPointField(output, this->GetOutputFieldName(), outputField);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool LagrangianStructures::DoMapField(
  svtkm::cont::DataSet&,
  const svtkm::cont::ArrayHandle<T, StorageType>&,
  const svtkm::filter::FieldMetadata&,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace svtkm::filter
#endif
