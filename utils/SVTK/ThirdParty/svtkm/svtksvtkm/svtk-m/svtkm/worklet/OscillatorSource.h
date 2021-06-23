//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_OscillatorSource_h
#define svtk_m_worklet_OscillatorSource_h

#include <svtkm/Math.h>
#include <svtkm/worklet/WorkletMapField.h>

#define MAX_OSCILLATORS 10

namespace svtkm
{
namespace worklet
{
namespace internal
{

struct Oscillator
{
  void Set(svtkm::Float64 x,
           svtkm::Float64 y,
           svtkm::Float64 z,
           svtkm::Float64 radius,
           svtkm::Float64 omega,
           svtkm::Float64 zeta)
  {
    this->Center[0] = x;
    this->Center[1] = y;
    this->Center[2] = z;
    this->Radius = radius;
    this->Omega = omega;
    this->Zeta = zeta;
  }

  svtkm::Vec3f_64 Center;
  svtkm::Float64 Radius;
  svtkm::Float64 Omega;
  svtkm::Float64 Zeta;
};
}

class OscillatorSource : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  SVTKM_CONT
  OscillatorSource()
    : NumberOfPeriodics(0)
    , NumberOfDamped(0)
    , NumberOfDecaying(0)
  {
  }

  SVTKM_CONT
  void AddPeriodic(svtkm::Float64 x,
                   svtkm::Float64 y,
                   svtkm::Float64 z,
                   svtkm::Float64 radius,
                   svtkm::Float64 omega,
                   svtkm::Float64 zeta)
  {
    if (this->NumberOfPeriodics < MAX_OSCILLATORS)
    {
      this->PeriodicOscillators[this->NumberOfPeriodics].Set(x, y, z, radius, omega, zeta);
      this->NumberOfPeriodics++;
    }
  }

  SVTKM_CONT
  void AddDamped(svtkm::Float64 x,
                 svtkm::Float64 y,
                 svtkm::Float64 z,
                 svtkm::Float64 radius,
                 svtkm::Float64 omega,
                 svtkm::Float64 zeta)
  {
    if (this->NumberOfDamped < MAX_OSCILLATORS)
    {
      this->DampedOscillators[this->NumberOfDamped * 6].Set(x, y, z, radius, omega, zeta);
      this->NumberOfDamped++;
    }
  }

  SVTKM_CONT
  void AddDecaying(svtkm::Float64 x,
                   svtkm::Float64 y,
                   svtkm::Float64 z,
                   svtkm::Float64 radius,
                   svtkm::Float64 omega,
                   svtkm::Float64 zeta)
  {
    if (this->NumberOfDecaying < MAX_OSCILLATORS)
    {
      this->DecayingOscillators[this->NumberOfDecaying * 6].Set(x, y, z, radius, omega, zeta);
      this->NumberOfDecaying++;
    }
  }

  SVTKM_CONT
  void SetTime(svtkm::Float64 time) { this->Time = time; }

  SVTKM_EXEC
  svtkm::Float64 operator()(const svtkm::Vec3f_64& vec) const
  {
    svtkm::UInt8 oIdx;
    svtkm::Float64 t0, t, result = 0;
    const internal::Oscillator* oscillator;

    t0 = 0.0;
    t = this->Time * 2 * 3.14159265358979323846;

    // Compute damped
    for (oIdx = 0; oIdx < this->NumberOfDamped; oIdx++)
    {
      oscillator = &this->DampedOscillators[oIdx];

      svtkm::Vec3f_64 delta = oscillator->Center - vec;
      svtkm::Float64 dist2 = dot(delta, delta);
      svtkm::Float64 dist_damp = svtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      svtkm::Float64 phi = svtkm::ACos(oscillator->Zeta);
      svtkm::Float64 val = 1. -
        svtkm::Exp(-oscillator->Zeta * oscillator->Omega * t0) *
          (svtkm::Sin(svtkm::Sqrt(1 - oscillator->Zeta * oscillator->Zeta) * oscillator->Omega * t +
                     phi) /
           svtkm::Sin(phi));
      result += val * dist_damp;
    }

    // Compute decaying
    for (oIdx = 0; oIdx < this->NumberOfDecaying; oIdx++)
    {
      oscillator = &this->DecayingOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      svtkm::Vec3f_64 delta = oscillator->Center - vec;
      svtkm::Float64 dist2 = dot(delta, delta);
      svtkm::Float64 dist_damp = svtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      svtkm::Float64 val = svtkm::Sin(t / oscillator->Omega) / (oscillator->Omega * t);
      result += val * dist_damp;
    }

    // Compute periodic
    for (oIdx = 0; oIdx < this->NumberOfPeriodics; oIdx++)
    {
      oscillator = &this->PeriodicOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      svtkm::Vec3f_64 delta = oscillator->Center - vec;
      svtkm::Float64 dist2 = dot(delta, delta);
      svtkm::Float64 dist_damp = svtkm::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      svtkm::Float64 val = svtkm::Sin(t / oscillator->Omega);
      result += val * dist_damp;
    }

    // We are done...
    return result;
  }

  template <typename T>
  SVTKM_EXEC svtkm::Float64 operator()(const svtkm::Vec<T, 3>& vec) const
  {
    return (*this)(svtkm::make_Vec(static_cast<svtkm::Float64>(vec[0]),
                                  static_cast<svtkm::Float64>(vec[1]),
                                  static_cast<svtkm::Float64>(vec[2])));
  }

private:
  svtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> PeriodicOscillators;
  svtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> DampedOscillators;
  svtkm::Vec<internal::Oscillator, MAX_OSCILLATORS> DecayingOscillators;
  svtkm::UInt8 NumberOfPeriodics;
  svtkm::UInt8 NumberOfDamped;
  svtkm::UInt8 NumberOfDecaying;
  svtkm::Float64 Time;
};
}

} // namespace svtkm

#endif // svtk_m_worklet_PointElevation_h
