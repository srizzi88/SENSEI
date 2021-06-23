//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Streamline_hxx
#define svtk_m_filter_Streamline_hxx

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/Integrators.h>
#include <svtkm/worklet/particleadvection/Particles.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Streamline::Streamline()
  : svtkm::filter::FilterDataSetWithField<Streamline>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void Streamline::SetSeeds(svtkm::cont::ArrayHandle<svtkm::Particle>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Streamline::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //Check for some basics.
  if (this->Seeds.GetNumberOfValues() == 0)
  {
    throw svtkm::cont::ErrorFilterExecution("No seeds provided.");
  }

  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!fieldMeta.IsPointField())
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  using FieldHandle = svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = svtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = svtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  GridEvalType eval(coords, cells, field);
  RK4Type rk4(eval, this->StepSize);

  svtkm::worklet::StreamlineResult res;

  svtkm::cont::ArrayHandle<svtkm::Particle> seedArray;
  svtkm::cont::ArrayCopy(this->Seeds, seedArray);
  res = this->Worklet.Run(rk4, seedArray, this->NumberOfSteps);

  svtkm::cont::DataSet outData;
  svtkm::cont::CoordinateSystem outputCoords("coordinates", res.Positions);
  outData.SetCellSet(res.PolyLines);
  outData.AddCoordinateSystem(outputCoords);

  return outData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Streamline::DoMapField(svtkm::cont::DataSet&,
                                             const svtkm::cont::ArrayHandle<T, StorageType>&,
                                             const svtkm::filter::FieldMetadata&,
                                             svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace svtkm::filter
#endif
