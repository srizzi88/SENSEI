//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particleadvection_Particles_h
#define svtk_m_worklet_particleadvection_Particles_h

#include <svtkm/Particle.h>
#include <svtkm/Types.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/worklet/particleadvection/IntegratorStatus.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{
template <typename Device>
class ParticleExecutionObject
{
public:
  SVTKM_EXEC_CONT
  ParticleExecutionObject()
    : Particles()
    , MaxSteps(0)
  {
  }

  ParticleExecutionObject(svtkm::cont::ArrayHandle<svtkm::Particle> particleArray, svtkm::Id maxSteps)
  {
    Particles = particleArray.PrepareForInPlace(Device());
    MaxSteps = maxSteps;
  }

  SVTKM_EXEC
  svtkm::Particle GetParticle(const svtkm::Id& idx) { return this->Particles.Get(idx); }

  SVTKM_EXEC
  void PreStepUpdate(const svtkm::Id& svtkmNotUsed(idx)) {}

  SVTKM_EXEC
  void StepUpdate(const svtkm::Id& idx, svtkm::FloatDefault time, const svtkm::Vec3f& pt)
  {
    svtkm::Particle p = this->GetParticle(idx);
    p.Pos = pt;
    p.Time = time;
    p.NumSteps++;
    this->Particles.Set(idx, p);
  }

  SVTKM_EXEC
  void StatusUpdate(const svtkm::Id& idx,
                    const svtkm::worklet::particleadvection::IntegratorStatus& status,
                    svtkm::Id maxSteps)
  {
    svtkm::Particle p = this->GetParticle(idx);

    if (p.NumSteps == maxSteps)
      p.Status.SetTerminate();

    if (status.CheckFail())
      p.Status.SetFail();
    if (status.CheckSpatialBounds())
      p.Status.SetSpatialBounds();
    if (status.CheckTemporalBounds())
      p.Status.SetTemporalBounds();
    this->Particles.Set(idx, p);
  }

  SVTKM_EXEC
  bool CanContinue(const svtkm::Id& idx)
  {
    svtkm::Particle p = this->GetParticle(idx);

    return (p.Status.CheckOk() && !p.Status.CheckTerminate() && !p.Status.CheckSpatialBounds() &&
            !p.Status.CheckTemporalBounds());
  }

  SVTKM_EXEC
  void UpdateTookSteps(const svtkm::Id& idx, bool val)
  {
    svtkm::Particle p = this->GetParticle(idx);
    if (val)
      p.Status.SetTookAnySteps();
    else
      p.Status.ClearTookAnySteps();
    this->Particles.Set(idx, p);
  }

protected:
  using ParticlePortal =
    typename svtkm::cont::ArrayHandle<svtkm::Particle>::template ExecutionTypes<Device>::Portal;

  ParticlePortal Particles;
  svtkm::Id MaxSteps;
};

class Particles : public svtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  SVTKM_CONT svtkm::worklet::particleadvection::ParticleExecutionObject<Device> PrepareForExecution(
    Device) const
  {
    return svtkm::worklet::particleadvection::ParticleExecutionObject<Device>(this->ParticleArray,
                                                                             this->MaxSteps);
  }

  SVTKM_CONT
  Particles(svtkm::cont::ArrayHandle<svtkm::Particle>& pArray, svtkm::Id& maxSteps)
    : ParticleArray(pArray)
    , MaxSteps(maxSteps)
  {
  }

  Particles() {}

protected:
  svtkm::cont::ArrayHandle<svtkm::Particle> ParticleArray;
  svtkm::Id MaxSteps;
};


template <typename Device>
class StateRecordingParticleExecutionObject : public ParticleExecutionObject<Device>
{
public:
  SVTKM_EXEC_CONT
  StateRecordingParticleExecutionObject()
    : ParticleExecutionObject<Device>()
    , History()
    , Length(0)
    , StepCount()
    , ValidPoint()
  {
  }

  StateRecordingParticleExecutionObject(svtkm::cont::ArrayHandle<svtkm::Particle> pArray,
                                        svtkm::cont::ArrayHandle<svtkm::Vec3f> historyArray,
                                        svtkm::cont::ArrayHandle<svtkm::Id> validPointArray,
                                        svtkm::cont::ArrayHandle<svtkm::Id> stepCountArray,
                                        svtkm::Id maxSteps)
    : ParticleExecutionObject<Device>(pArray, maxSteps)
    , Length(maxSteps + 1)
  {
    svtkm::Id numPos = pArray.GetNumberOfValues();
    History = historyArray.PrepareForOutput(numPos * Length, Device());
    ValidPoint = validPointArray.PrepareForInPlace(Device());
    StepCount = stepCountArray.PrepareForInPlace(Device());
  }

  SVTKM_EXEC
  void PreStepUpdate(const svtkm::Id& idx)
  {
    svtkm::Particle p = this->ParticleExecutionObject<Device>::GetParticle(idx);
    if (p.NumSteps == 0)
    {
      svtkm::Id loc = idx * Length;
      this->History.Set(loc, p.Pos);
      this->ValidPoint.Set(loc, 1);
      this->StepCount.Set(idx, 1);
    }
  }

  SVTKM_EXEC
  void StepUpdate(const svtkm::Id& idx, svtkm::FloatDefault time, const svtkm::Vec3f& pt)
  {
    this->ParticleExecutionObject<Device>::StepUpdate(idx, time, pt);

    //local step count.
    svtkm::Id stepCount = this->StepCount.Get(idx);

    svtkm::Id loc = idx * Length + stepCount;
    this->History.Set(loc, pt);
    this->ValidPoint.Set(loc, 1);
    this->StepCount.Set(idx, stepCount + 1);
  }

protected:
  using IdPortal =
    typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<Device>::Portal;
  using HistoryPortal =
    typename svtkm::cont::ArrayHandle<svtkm::Vec3f>::template ExecutionTypes<Device>::Portal;

  HistoryPortal History;
  svtkm::Id Length;
  IdPortal StepCount;
  IdPortal ValidPoint;
};

class StateRecordingParticles : svtkm::cont::ExecutionObjectBase
{
public:
  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    SVTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };


  template <typename Device>
  SVTKM_CONT svtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<Device>
    PrepareForExecution(Device) const
  {
    return svtkm::worklet::particleadvection::StateRecordingParticleExecutionObject<Device>(
      this->ParticleArray,
      this->HistoryArray,
      this->ValidPointArray,
      this->StepCountArray,
      this->MaxSteps);
  }
  SVTKM_CONT
  StateRecordingParticles(svtkm::cont::ArrayHandle<svtkm::Particle>& pArray, const svtkm::Id& maxSteps)
    : MaxSteps(maxSteps)
    , ParticleArray(pArray)
  {
    svtkm::Id numParticles = static_cast<svtkm::Id>(pArray.GetNumberOfValues());

    //Create ValidPointArray initialized to zero.
    svtkm::cont::ArrayHandleConstant<svtkm::Id> tmp(0, (this->MaxSteps + 1) * numParticles);
    svtkm::cont::ArrayCopy(tmp, this->ValidPointArray);

    //Create StepCountArray initialized to zero.
    svtkm::cont::ArrayHandleConstant<svtkm::Id> tmp2(0, numParticles);
    svtkm::cont::ArrayCopy(tmp2, this->StepCountArray);
  }

  SVTKM_CONT
  StateRecordingParticles(svtkm::cont::ArrayHandle<svtkm::Particle>& pArray,
                          svtkm::cont::ArrayHandle<svtkm::Vec3f>& historyArray,
                          svtkm::cont::ArrayHandle<svtkm::Id>& validPointArray,
                          svtkm::Id& maxSteps)
  {
    ParticleArray = pArray;
    HistoryArray = historyArray;
    ValidPointArray = validPointArray;
    MaxSteps = maxSteps;
  }

  SVTKM_CONT
  void GetCompactedHistory(svtkm::cont::ArrayHandle<svtkm::Vec3f>& positions)
  {
    svtkm::cont::Algorithm::CopyIf(this->HistoryArray, this->ValidPointArray, positions, IsOne());
  }

protected:
  svtkm::cont::ArrayHandle<svtkm::Vec3f> HistoryArray;
  svtkm::Id MaxSteps;
  svtkm::cont::ArrayHandle<svtkm::Particle> ParticleArray;
  svtkm::cont::ArrayHandle<svtkm::Id> StepCountArray;
  svtkm::cont::ArrayHandle<svtkm::Id> ValidPointArray;
};


} //namespace particleadvection
} //namespace worklet
} //namespace svtkm

#endif // svtk_m_worklet_particleadvection_Particles_h
//============================================================================
