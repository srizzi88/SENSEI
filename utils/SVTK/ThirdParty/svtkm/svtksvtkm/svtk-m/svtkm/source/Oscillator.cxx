//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/source/Oscillator.h>

namespace svtkm
{
namespace source
{

//-----------------------------------------------------------------------------
Oscillator::Oscillator(svtkm::Id3 dims)
  : Dims(dims)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
void Oscillator::SetTime(svtkm::Float64 time)
{
  this->Worklet.SetTime(time);
}

//-----------------------------------------------------------------------------
void Oscillator::AddPeriodic(svtkm::Float64 x,
                             svtkm::Float64 y,
                             svtkm::Float64 z,
                             svtkm::Float64 radius,
                             svtkm::Float64 omega,
                             svtkm::Float64 zeta)
{
  this->Worklet.AddPeriodic(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDamped(svtkm::Float64 x,
                           svtkm::Float64 y,
                           svtkm::Float64 z,
                           svtkm::Float64 radius,
                           svtkm::Float64 omega,
                           svtkm::Float64 zeta)
{
  this->Worklet.AddDamped(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDecaying(svtkm::Float64 x,
                             svtkm::Float64 y,
                             svtkm::Float64 z,
                             svtkm::Float64 radius,
                             svtkm::Float64 omega,
                             svtkm::Float64 zeta)
{
  this->Worklet.AddDecaying(x, y, z, radius, omega, zeta);
}


//-----------------------------------------------------------------------------
svtkm::cont::DataSet Oscillator::Execute() const
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

  svtkm::cont::DataSet dataSet;

  svtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(this->Dims);
  dataSet.SetCellSet(cellSet);

  const svtkm::Vec3f origin(0.0f, 0.0f, 0.0f);
  const svtkm::Vec3f spacing(1.0f / static_cast<svtkm::FloatDefault>(this->Dims[0]),
                            1.0f / static_cast<svtkm::FloatDefault>(this->Dims[1]),
                            1.0f / static_cast<svtkm::FloatDefault>(this->Dims[2]));

  const svtkm::Id3 pdims{ this->Dims + svtkm::Id3{ 1, 1, 1 } };
  svtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pdims, origin, spacing);
  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));


  svtkm::cont::ArrayHandle<svtkm::Float64> outArray;
  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  this->Invoke(this->Worklet, coordinates, outArray);
  dataSet.AddField(svtkm::cont::make_FieldPoint("scalars", outArray));

  return dataSet;
}
}
} // namespace svtkm::filter
