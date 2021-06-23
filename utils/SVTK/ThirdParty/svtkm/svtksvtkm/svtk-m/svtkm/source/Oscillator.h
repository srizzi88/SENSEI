//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_source_OscillatorSource_h
#define svtk_m_source_OscillatorSource_h

#include <svtkm/source/Source.h>
#include <svtkm/worklet/OscillatorSource.h>

namespace svtkm
{
namespace source
{

/**\brief An analytical, time-varying uniform dataset with a point based array
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * This array is based on the coordinates and evaluates to a sum of time-varying
 * Gaussian exponentials specified in its configuration.
 */
class SVTKM_SOURCE_EXPORT Oscillator final : public svtkm::source::Source
{
public:
  ///Construct a Oscillator with Cell Dimensions
  SVTKM_CONT
  Oscillator(svtkm::Id3 dims);

  SVTKM_CONT
  void SetTime(svtkm::Float64 time);

  SVTKM_CONT
  void AddPeriodic(svtkm::Float64 x,
                   svtkm::Float64 y,
                   svtkm::Float64 z,
                   svtkm::Float64 radius,
                   svtkm::Float64 omega,
                   svtkm::Float64 zeta);

  SVTKM_CONT
  void AddDamped(svtkm::Float64 x,
                 svtkm::Float64 y,
                 svtkm::Float64 z,
                 svtkm::Float64 radius,
                 svtkm::Float64 omega,
                 svtkm::Float64 zeta);

  SVTKM_CONT
  void AddDecaying(svtkm::Float64 x,
                   svtkm::Float64 y,
                   svtkm::Float64 z,
                   svtkm::Float64 radius,
                   svtkm::Float64 omega,
                   svtkm::Float64 zeta);

  SVTKM_CONT svtkm::cont::DataSet Execute() const;

private:
  svtkm::Id3 Dims;
  svtkm::worklet::OscillatorSource Worklet;
};
}
}

#endif // svtk_m_source_Oscillator_h
