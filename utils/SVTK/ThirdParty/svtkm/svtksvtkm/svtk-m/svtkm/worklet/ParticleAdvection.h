//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_ParticleAdvection_h
#define svtk_m_worklet_ParticleAdvection_h

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/particleadvection/ParticleAdvectionWorklets.h>

namespace svtkm
{
namespace worklet
{

namespace detail
{
class CopyToParticle : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn pt, FieldIn id, FieldIn time, FieldIn step, FieldOut particle);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  SVTKM_EXEC void operator()(const svtkm::Vec3f& pt,
                            const svtkm::Id& id,
                            const svtkm::FloatDefault& time,
                            const svtkm::Id& step,
                            svtkm::Particle& particle) const
  {
    particle.Pos = pt;
    particle.ID = id;
    particle.Time = time;
    particle.NumSteps = step;
    particle.Status.SetOk();
  }
};

} //detail

struct ParticleAdvectionResult
{
  ParticleAdvectionResult()
    : Particles()
  {
  }

  ParticleAdvectionResult(const svtkm::cont::ArrayHandle<svtkm::Particle>& p)
    : Particles(p)
  {
  }

  svtkm::cont::ArrayHandle<svtkm::Particle> Particles;
};

class ParticleAdvection
{
public:
  ParticleAdvection() {}

  template <typename IntegratorType, typename ParticleStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              svtkm::cont::ArrayHandle<svtkm::Particle, ParticleStorage>& particles,
                              svtkm::Id MaxSteps)
  {
    svtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType> worklet;

    worklet.Run(it, particles, MaxSteps);
    return ParticleAdvectionResult(particles);
  }

  template <typename IntegratorType, typename PointStorage>
  ParticleAdvectionResult Run(const IntegratorType& it,
                              const svtkm::cont::ArrayHandle<svtkm::Vec3f, PointStorage>& points,
                              svtkm::Id MaxSteps)
  {
    svtkm::worklet::particleadvection::ParticleAdvectionWorklet<IntegratorType> worklet;

    svtkm::cont::ArrayHandle<svtkm::Particle> particles;
    svtkm::cont::ArrayHandle<svtkm::Id> step, ids;
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> time;
    svtkm::cont::Invoker invoke;

    svtkm::Id numPts = points.GetNumberOfValues();
    svtkm::cont::ArrayHandleConstant<svtkm::Id> s(0, numPts);
    svtkm::cont::ArrayHandleConstant<svtkm::FloatDefault> t(0, numPts);
    svtkm::cont::ArrayHandleCounting<svtkm::Id> id(0, 1, numPts);

    //Copy input to svtkm::Particle
    svtkm::cont::ArrayCopy(s, step);
    svtkm::cont::ArrayCopy(t, time);
    svtkm::cont::ArrayCopy(id, ids);
    invoke(detail::CopyToParticle{}, points, ids, time, step, particles);

    worklet.Run(it, particles, MaxSteps);
    return ParticleAdvectionResult(particles);
  }
};

struct StreamlineResult
{
  StreamlineResult()
    : Particles()
    , Positions()
    , PolyLines()
  {
  }

  StreamlineResult(const svtkm::cont::ArrayHandle<svtkm::Particle>& part,
                   const svtkm::cont::ArrayHandle<svtkm::Vec3f>& pos,
                   const svtkm::cont::CellSetExplicit<>& lines)
    : Particles(part)
    , Positions(pos)
    , PolyLines(lines)
  {
  }

  svtkm::cont::ArrayHandle<svtkm::Particle> Particles;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> Positions;
  svtkm::cont::CellSetExplicit<> PolyLines;
};

class Streamline
{
public:
  Streamline() {}

  template <typename IntegratorType, typename ParticleStorage>
  StreamlineResult Run(const IntegratorType& it,
                       svtkm::cont::ArrayHandle<svtkm::Particle, ParticleStorage>& particles,
                       svtkm::Id MaxSteps)
  {
    svtkm::worklet::particleadvection::StreamlineWorklet<IntegratorType> worklet;

    svtkm::cont::ArrayHandle<svtkm::Vec3f> positions;
    svtkm::cont::CellSetExplicit<> polyLines;

    worklet.Run(it, particles, MaxSteps, positions, polyLines);

    return StreamlineResult(particles, positions, polyLines);
  }
};
}
}

#endif // svtk_m_worklet_ParticleAdvection_h
