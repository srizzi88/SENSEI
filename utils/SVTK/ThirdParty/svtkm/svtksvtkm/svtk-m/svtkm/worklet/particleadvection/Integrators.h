//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef svtk_m_worklet_particleadvection_Integrators_h
#define svtk_m_worklet_particleadvection_Integrators_h

#include <iomanip>
#include <limits>

#include <svtkm/Bitset.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VectorAnalysis.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/VirtualObjectHandle.h>

#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/IntegratorStatus.h>
#include <svtkm/worklet/particleadvection/Particles.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{

class Integrator : public svtkm::cont::ExecutionObjectBase
{
protected:
  SVTKM_CONT
  Integrator() = default;

  SVTKM_CONT
  Integrator(svtkm::FloatDefault stepLength)
    : StepLength(stepLength)
  {
  }

public:
  class ExecObject : public svtkm::VirtualObjectBase
  {
  protected:
    SVTKM_EXEC_CONT
    ExecObject(const svtkm::FloatDefault stepLength, svtkm::FloatDefault tolerance)
      : StepLength(stepLength)
      , Tolerance(tolerance)
    {
    }

  public:
    SVTKM_EXEC
    virtual IntegratorStatus Step(const svtkm::Vec3f& inpos,
                                  svtkm::FloatDefault& time,
                                  svtkm::Vec3f& outpos) const = 0;

    SVTKM_EXEC
    virtual IntegratorStatus SmallStep(svtkm::Vec3f& inpos,
                                       svtkm::FloatDefault& time,
                                       svtkm::Vec3f& outpos) const = 0;

  protected:
    svtkm::FloatDefault StepLength = 1.0f;
    svtkm::FloatDefault Tolerance = 0.001f;
  };

  template <typename Device>
  SVTKM_CONT const ExecObject* PrepareForExecution(Device) const
  {
    this->PrepareForExecutionImpl(
      Device(), const_cast<svtkm::cont::VirtualObjectHandle<ExecObject>&>(this->ExecObjectHandle));
    return this->ExecObjectHandle.PrepareForExecution(Device());
  }

private:
  svtkm::cont::VirtualObjectHandle<ExecObject> ExecObjectHandle;

protected:
  svtkm::FloatDefault StepLength;
  svtkm::FloatDefault Tolerance =
    std::numeric_limits<svtkm::FloatDefault>::epsilon() * static_cast<svtkm::FloatDefault>(100.0f);

  SVTKM_CONT virtual void PrepareForExecutionImpl(
    svtkm::cont::DeviceAdapterId device,
    svtkm::cont::VirtualObjectHandle<ExecObject>& execObjectHandle) const = 0;

  template <typename FieldEvaluateType, typename DerivedType>
  class ExecObjectBaseImpl : public ExecObject
  {
  protected:
    SVTKM_EXEC_CONT
    ExecObjectBaseImpl(const FieldEvaluateType& evaluator,
                       svtkm::FloatDefault stepLength,
                       svtkm::FloatDefault tolerance)
      : ExecObject(stepLength, tolerance)
      , Evaluator(evaluator)
    {
    }

  public:
    SVTKM_EXEC
    IntegratorStatus Step(const svtkm::Vec3f& inpos,
                          svtkm::FloatDefault& time,
                          svtkm::Vec3f& outpos) const override
    {
      // If particle is out of either spatial or temporal boundary to begin with,
      // then return the corresponding status.
      IntegratorStatus status;
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
      {
        status.SetFail();
        status.SetSpatialBounds();
        return status;
      }
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
      {
        status.SetFail();
        status.SetTemporalBounds();
        return status;
      }

      svtkm::Vec3f velocity;
      status = CheckStep(inpos, this->StepLength, time, velocity);
      if (status.CheckOk())
      {
        outpos = inpos + StepLength * velocity;
        time += StepLength;
      }
      else
        outpos = inpos;

      return status;
    }

    SVTKM_EXEC
    IntegratorStatus SmallStep(svtkm::Vec3f& inpos,
                               svtkm::FloatDefault& time,
                               svtkm::Vec3f& outpos) const override
    {
      if (!this->Evaluator.IsWithinSpatialBoundary(inpos))
      {
        outpos = inpos;
        return IntegratorStatus(false, true, false);
      }
      if (!this->Evaluator.IsWithinTemporalBoundary(time))
      {
        outpos = inpos;
        return IntegratorStatus(false, false, true);
      }

      //Stepping by this->StepLength goes beyond the bounds of the dataset.
      //We need to take an Euler step that goes outside of the dataset.
      //Use a binary search to find the largest step INSIDE the dataset.
      //Binary search uses a shrinking bracket of inside / outside, so when
      //we terminate, the outside value is the stepsize that will nudge
      //the particle outside the dataset.

      //The binary search will be between [0, this->StepLength]
      svtkm::FloatDefault stepShort = 0, stepLong = this->StepLength;
      svtkm::Vec3f currPos(inpos), velocity;
      svtkm::FloatDefault currTime = time;

      auto evalStatus = this->Evaluator.Evaluate(currPos, time, velocity);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);

      const svtkm::FloatDefault eps = svtkm::Epsilon<svtkm::FloatDefault>();
      svtkm::FloatDefault div = 1;
      for (int i = 0; i < 50; i++)
      {
        div *= 2;
        svtkm::FloatDefault stepCurr = stepShort + (this->StepLength / div);
        //See if we can step by stepCurr.
        IntegratorStatus status = this->CheckStep(inpos, stepCurr, time, velocity);
        if (status.CheckOk())
        {
          currPos = inpos + velocity * stepShort;
          stepShort = stepCurr;
        }
        else
          stepLong = stepCurr;

        //Stop if step bracket is small enough.
        if (stepLong - stepShort < eps)
          break;
      }

      //Take Euler step.
      currTime = time + stepShort;
      evalStatus = this->Evaluator.Evaluate(currPos, currTime, velocity);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);

      outpos = currPos + stepLong * velocity;
      return IntegratorStatus(true, true, !this->Evaluator.IsWithinTemporalBoundary(time));
    }

    SVTKM_EXEC
    IntegratorStatus CheckStep(const svtkm::Vec3f& inpos,
                               svtkm::FloatDefault stepLength,
                               svtkm::FloatDefault time,
                               svtkm::Vec3f& velocity) const
    {
      return static_cast<const DerivedType*>(this)->CheckStep(inpos, stepLength, time, velocity);
    }

  protected:
    FieldEvaluateType Evaluator;
  };
};

namespace detail
{

template <template <typename> class IntegratorType>
struct IntegratorPrepareForExecutionFunctor
{
  template <typename Device, typename EvaluatorType>
  SVTKM_CONT bool operator()(
    Device,
    svtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle,
    const EvaluatorType& evaluator,
    svtkm::FloatDefault stepLength,
    svtkm::FloatDefault tolerance) const
  {
    IntegratorType<Device>* integrator =
      new IntegratorType<Device>(evaluator.PrepareForExecution(Device()), stepLength, tolerance);
    execObjectHandle.Reset(integrator);
    return true;
  }
};

} // namespace detail

template <typename FieldEvaluateType>
class RK4Integrator : public Integrator
{
public:
  SVTKM_CONT
  RK4Integrator() = default;

  SVTKM_CONT
  RK4Integrator(const FieldEvaluateType& evaluator, svtkm::FloatDefault stepLength)
    : Integrator(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject : public Integrator::ExecObjectBaseImpl<
                       decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device())),
                       typename RK4Integrator::template ExecObject<Device>>
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device()));
    using Superclass =
      Integrator::ExecObjectBaseImpl<FieldEvaluateExecType,
                                     typename RK4Integrator::template ExecObject<Device>>;

  public:
    SVTKM_EXEC_CONT
    ExecObject(const FieldEvaluateExecType& evaluator,
               svtkm::FloatDefault stepLength,
               svtkm::FloatDefault tolerance)
      : Superclass(evaluator, stepLength, tolerance)
    {
    }

    SVTKM_EXEC
    IntegratorStatus CheckStep(const svtkm::Vec3f& inpos,
                               svtkm::FloatDefault stepLength,
                               svtkm::FloatDefault time,
                               svtkm::Vec3f& velocity) const
    {
      svtkm::FloatDefault boundary = this->Evaluator.GetTemporalBoundary(static_cast<svtkm::Id>(1));
      if ((time + stepLength + svtkm::Epsilon<svtkm::FloatDefault>() - boundary) > 0.0)
        stepLength = boundary - time;

      svtkm::FloatDefault var1 = (stepLength / static_cast<svtkm::FloatDefault>(2));
      svtkm::FloatDefault var2 = time + var1;
      svtkm::FloatDefault var3 = time + stepLength;

      svtkm::Vec3f k1 = svtkm::TypeTraits<svtkm::Vec3f>::ZeroInitialization();
      svtkm::Vec3f k2 = k1, k3 = k1, k4 = k1;

      GridEvaluatorStatus evalStatus;
      evalStatus = this->Evaluator.Evaluate(inpos, time, k1);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      evalStatus = this->Evaluator.Evaluate(inpos + var1 * k1, var2, k2);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      evalStatus = this->Evaluator.Evaluate(inpos + var1 * k2, var2, k3);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);
      evalStatus = this->Evaluator.Evaluate(inpos + stepLength * k3, var3, k4);
      if (evalStatus.CheckFail())
        return IntegratorStatus(evalStatus);

      velocity = (k1 + 2 * k2 + 2 * k3 + k4) / 6.0f;
      return IntegratorStatus(true, false, evalStatus.CheckTemporalBounds());
    }
  };

private:
  FieldEvaluateType Evaluator;

protected:
  SVTKM_CONT virtual void PrepareForExecutionImpl(
    svtkm::cont::DeviceAdapterId device,
    svtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle) const override
  {
    svtkm::cont::TryExecuteOnDevice(device,
                                   detail::IntegratorPrepareForExecutionFunctor<ExecObject>(),
                                   execObjectHandle,
                                   this->Evaluator,
                                   this->StepLength,
                                   this->Tolerance);
  }
};

template <typename FieldEvaluateType>
class EulerIntegrator : public Integrator
{
public:
  EulerIntegrator() = default;

  SVTKM_CONT
  EulerIntegrator(const FieldEvaluateType& evaluator, const svtkm::FloatDefault stepLength)
    : Integrator(stepLength)
    , Evaluator(evaluator)
  {
  }

  template <typename Device>
  class ExecObject : public Integrator::ExecObjectBaseImpl<
                       decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device())),
                       typename EulerIntegrator::template ExecObject<Device>>
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using FieldEvaluateExecType =
      decltype(std::declval<FieldEvaluateType>().PrepareForExecution(Device()));
    using Superclass =
      Integrator::ExecObjectBaseImpl<FieldEvaluateExecType,
                                     typename EulerIntegrator::template ExecObject<Device>>;

  public:
    SVTKM_EXEC_CONT
    ExecObject(const FieldEvaluateExecType& evaluator,
               svtkm::FloatDefault stepLength,
               svtkm::FloatDefault tolerance)
      : Superclass(evaluator, stepLength, tolerance)
    {
    }

    SVTKM_EXEC
    IntegratorStatus CheckStep(const svtkm::Vec3f& inpos,
                               svtkm::FloatDefault svtkmNotUsed(stepLength),
                               svtkm::FloatDefault time,
                               svtkm::Vec3f& velocity) const
    {
      GridEvaluatorStatus status = this->Evaluator.Evaluate(inpos, time, velocity);
      return IntegratorStatus(status);
    }
  };

private:
  FieldEvaluateType Evaluator;

protected:
  SVTKM_CONT virtual void PrepareForExecutionImpl(
    svtkm::cont::DeviceAdapterId device,
    svtkm::cont::VirtualObjectHandle<Integrator::ExecObject>& execObjectHandle) const override
  {
    svtkm::cont::TryExecuteOnDevice(device,
                                   detail::IntegratorPrepareForExecutionFunctor<ExecObject>(),
                                   execObjectHandle,
                                   this->Evaluator,
                                   this->StepLength,
                                   this->Tolerance);
  }
}; //EulerIntegrator

} //namespace particleadvection
} //namespace worklet
} //namespace svtkm

#endif // svtk_m_worklet_particleadvection_Integrators_h
