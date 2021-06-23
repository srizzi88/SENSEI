//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_Particle_h
#define svtk_m_Particle_h

#include <svtkm/Bitset.h>

namespace svtkm
{

//Bit field describing the status:
class ParticleStatus : public svtkm::Bitset<svtkm::UInt8>
{
public:
  SVTKM_EXEC_CONT ParticleStatus()
  {
    this->SetOk();
    this->ClearTerminate();
  }

  SVTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  SVTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  SVTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  SVTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  SVTKM_EXEC_CONT void SetTerminate() { this->set(this->TERMINATE_BIT); }
  SVTKM_EXEC_CONT void ClearTerminate() { this->reset(this->TERMINATE_BIT); }
  SVTKM_EXEC_CONT bool CheckTerminate() const { return this->test(this->TERMINATE_BIT); }

  SVTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT void ClearSpatialBounds() { this->reset(this->SPATIAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  SVTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT void ClearTemporalBounds() { this->reset(this->TEMPORAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

  SVTKM_EXEC_CONT void SetTookAnySteps() { this->set(this->TOOK_ANY_STEPS_BIT); }
  SVTKM_EXEC_CONT void ClearTookAnySteps() { this->reset(this->TOOK_ANY_STEPS_BIT); }
  SVTKM_EXEC_CONT bool CheckTookAnySteps() const { return this->test(this->TOOK_ANY_STEPS_BIT); }

private:
  static constexpr svtkm::Id SUCCESS_BIT = 0;
  static constexpr svtkm::Id TERMINATE_BIT = 1;
  static constexpr svtkm::Id SPATIAL_BOUNDS_BIT = 2;
  static constexpr svtkm::Id TEMPORAL_BOUNDS_BIT = 3;
  static constexpr svtkm::Id TOOK_ANY_STEPS_BIT = 4;
};

inline SVTKM_CONT std::ostream& operator<<(std::ostream& s, const svtkm::ParticleStatus& status)
{
  s << "[" << status.CheckOk() << " " << status.CheckTerminate() << " "
    << status.CheckSpatialBounds() << " " << status.CheckTemporalBounds() << "]";
  return s;
}

class Particle
{
public:
  SVTKM_EXEC_CONT
  Particle() {}

  SVTKM_EXEC_CONT
  Particle(const svtkm::Vec3f& p,
           const svtkm::Id& id,
           const svtkm::Id& numSteps = 0,
           const svtkm::ParticleStatus& status = svtkm::ParticleStatus(),
           const svtkm::FloatDefault& time = 0)
    : Pos(p)
    , ID(id)
    , NumSteps(numSteps)
    , Status(status)
    , Time(time)
  {
  }

  SVTKM_EXEC_CONT
  Particle(const svtkm::Particle& p)
    : Pos(p.Pos)
    , ID(p.ID)
    , NumSteps(p.NumSteps)
    , Status(p.Status)
    , Time(p.Time)
  {
  }

  svtkm::Particle& operator=(const svtkm::Particle& p) = default;

  svtkm::Vec3f Pos;
  svtkm::Id ID = -1;
  svtkm::Id NumSteps = 0;
  svtkm::ParticleStatus Status;
  svtkm::FloatDefault Time = 0;
};
}

#endif // svtk_m_Particle_h
