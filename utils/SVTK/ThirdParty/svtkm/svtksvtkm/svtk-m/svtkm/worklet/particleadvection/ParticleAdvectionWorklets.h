//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
#define svtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h

#include <svtkm/Types.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/ExecutionObjectBase.h>

#include <svtkm/Particle.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/particleadvection/Integrators.h>
#include <svtkm/worklet/particleadvection/Particles.h>
//#include <svtkm/worklet/particleadvection/ParticlesAOS.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{

class ParticleAdvectWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn idx,
                                ExecObject integrator,
                                ExecObject integralCurve,
                                FieldIn maxSteps);
  using ExecutionSignature = void(_1 idx, _2 integrator, _3 integralCurve, _4 maxSteps);
  using InputDomain = _1;

  template <typename IntegratorType, typename IntegralCurveType>
  SVTKM_EXEC void operator()(const svtkm::Id& idx,
                            const IntegratorType* integrator,
                            IntegralCurveType& integralCurve,
                            const svtkm::Id& maxSteps) const
  {
    svtkm::Particle particle = integralCurve.GetParticle(idx);

    svtkm::Vec3f inpos = particle.Pos;
    svtkm::FloatDefault time = particle.Time;
    bool tookAnySteps = false;

    //the integrator status needs to be more robust:
    // 1. you could have success AND at temporal boundary.
    // 2. could you have success AND at spatial?
    // 3. all three?

    integralCurve.PreStepUpdate(idx);
    do
    {
      //svtkm::Particle p = integralCurve.GetParticle(idx);
      //std::cout<<idx<<": "<<inpos<<" #"<<p.NumSteps<<" "<<p.Status<<std::endl;
      svtkm::Vec3f outpos;
      auto status = integrator->Step(inpos, time, outpos);
      if (status.CheckOk())
      {
        integralCurve.StepUpdate(idx, time, outpos);
        tookAnySteps = true;
        inpos = outpos;
      }

      //We can't take a step inside spatial boundary.
      //Try and take a step just past the boundary.
      else if (status.CheckSpatialBounds())
      {
        IntegratorStatus status2 = integrator->SmallStep(inpos, time, outpos);
        if (status2.CheckOk())
        {
          integralCurve.StepUpdate(idx, time, outpos);
          tookAnySteps = true;

          //we took a step, so use this status to consider below.
          status = status2;
        }
      }

      integralCurve.StatusUpdate(idx, status, maxSteps);

    } while (integralCurve.CanContinue(idx));

    //Mark if any steps taken
    integralCurve.UpdateTookSteps(idx, tookAnySteps);

    //particle = integralCurve.GetParticle(idx);
    //std::cout<<idx<<": "<<inpos<<" #"<<particle.NumSteps<<" "<<particle.Status<<std::endl;
  }
};


template <typename IntegratorType>
class ParticleAdvectionWorklet
{
public:
  SVTKM_EXEC_CONT ParticleAdvectionWorklet() {}

  ~ParticleAdvectionWorklet() {}

  void Run(const IntegratorType& integrator,
           svtkm::cont::ArrayHandle<svtkm::Particle>& particles,
           svtkm::Id& MaxSteps)
  {

    using ParticleAdvectWorkletType = svtkm::worklet::particleadvection::ParticleAdvectWorklet;
    using ParticleWorkletDispatchType =
      typename svtkm::worklet::DispatcherMapField<ParticleAdvectWorkletType>;
    using ParticleType = svtkm::worklet::particleadvection::Particles;

    svtkm::Id numSeeds = static_cast<svtkm::Id>(particles.GetNumberOfValues());
    //Create and invoke the particle advection.
    svtkm::cont::ArrayHandleConstant<svtkm::Id> maxSteps(MaxSteps, numSeeds);
    svtkm::cont::ArrayHandleIndex idxArray(numSeeds);

#ifdef SVTKM_CUDA
    // This worklet needs some extra space on CUDA.
    svtkm::cont::cuda::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // SVTKM_CUDA

    ParticleType particlesObj(particles, MaxSteps);

    //Invoke particle advection worklet
    ParticleWorkletDispatchType particleWorkletDispatch;

    particleWorkletDispatch.Invoke(idxArray, integrator, particlesObj, maxSteps);
  }
};

namespace detail
{
class GetSteps : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  GetSteps() {}
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  SVTKM_EXEC void operator()(const svtkm::Particle& p, svtkm::Id& numSteps) const
  {
    numSteps = p.NumSteps;
  }
};

class ComputeNumPoints : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  ComputeNumPoints() {}
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  // Offset is number of points in streamline.
  // 1 (inital point) + number of steps taken (p.NumSteps - initalNumSteps)
  SVTKM_EXEC void operator()(const svtkm::Particle& p,
                            const svtkm::Id& initialNumSteps,
                            svtkm::Id& diff) const
  {
    diff = 1 + p.NumSteps - initialNumSteps;
  }
};
} // namespace detail


template <typename IntegratorType>
class StreamlineWorklet
{
public:
  template <typename PointStorage, typename PointStorage2>
  void Run(const IntegratorType& it,
           svtkm::cont::ArrayHandle<svtkm::Particle, PointStorage>& particles,
           svtkm::Id& MaxSteps,
           svtkm::cont::ArrayHandle<svtkm::Vec3f, PointStorage2>& positions,
           svtkm::cont::CellSetExplicit<>& polyLines)
  {

    using ParticleWorkletDispatchType = typename svtkm::worklet::DispatcherMapField<
      svtkm::worklet::particleadvection::ParticleAdvectWorklet>;
    using StreamlineType = svtkm::worklet::particleadvection::StateRecordingParticles;

    svtkm::cont::ArrayHandle<svtkm::Id> initialStepsTaken;

    svtkm::Id numSeeds = static_cast<svtkm::Id>(particles.GetNumberOfValues());
    svtkm::cont::ArrayHandleIndex idxArray(numSeeds);

    svtkm::worklet::DispatcherMapField<detail::GetSteps> getStepDispatcher{ (detail::GetSteps{}) };
    getStepDispatcher.Invoke(particles, initialStepsTaken);

#ifdef SVTKM_CUDA
    // This worklet needs some extra space on CUDA.
    svtkm::cont::cuda::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // SVTKM_CUDA

    //Run streamline worklet
    StreamlineType streamlines(particles, MaxSteps);
    ParticleWorkletDispatchType particleWorkletDispatch;
    svtkm::cont::ArrayHandleConstant<svtkm::Id> maxSteps(MaxSteps, numSeeds);
    particleWorkletDispatch.Invoke(idxArray, it, streamlines, maxSteps);

    //Get the positions
    streamlines.GetCompactedHistory(positions);

    //Create the cells
    svtkm::cont::ArrayHandle<svtkm::Id> numPoints;
    svtkm::worklet::DispatcherMapField<detail::ComputeNumPoints> computeNumPointsDispatcher{ (
      detail::ComputeNumPoints{}) };
    computeNumPointsDispatcher.Invoke(particles, initialStepsTaken, numPoints);

    svtkm::cont::ArrayHandle<svtkm::Id> cellIndex;
    svtkm::Id connectivityLen = svtkm::cont::Algorithm::ScanExclusive(numPoints, cellIndex);
    svtkm::cont::ArrayHandleCounting<svtkm::Id> connCount(0, 1, connectivityLen);
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    svtkm::cont::ArrayCopy(connCount, connectivity);

    svtkm::cont::ArrayHandle<svtkm::UInt8> cellTypes;
    auto polyLineShape =
      svtkm::cont::make_ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_POLY_LINE, numSeeds);
    svtkm::cont::ArrayCopy(polyLineShape, cellTypes);

    auto numIndices = svtkm::cont::make_ArrayHandleCast(numPoints, svtkm::IdComponent());
    auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);
    polyLines.Fill(positions.GetNumberOfValues(), cellTypes, connectivity, offsets);
  }
};
}
}
} // namespace svtkm::worklet::particleadvection

#endif // svtk_m_worklet_particleadvection_ParticleAdvectionWorklets_h
