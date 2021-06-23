//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_GradientOutput_h
#define svtk_m_worklet_gradient_GradientOutput_h

#include <svtkm/VecTraits.h>

#include <svtkm/cont/arg/TransportTagArrayOut.h>
#include <svtkm/cont/arg/TransportTagExecObject.h>

#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <svtkm/worklet/gradient/Divergence.h>
#include <svtkm/worklet/gradient/QCriterion.h>
#include <svtkm/worklet/gradient/Vorticity.h>

namespace svtkm
{
namespace exec
{
template <typename T, typename DeviceAdapter>
struct GradientScalarOutputExecutionObject
{
  using ValueType = svtkm::Vec<T, 3>;
  using BaseTType = typename svtkm::VecTraits<T>::BaseComponentType;

  struct PortalTypes
  {
    using HandleType = svtkm::cont::ArrayHandle<ValueType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

  GradientScalarOutputExecutionObject() = default;

  GradientScalarOutputExecutionObject(svtkm::cont::ArrayHandle<ValueType> gradient, svtkm::Id size)
  {
    this->GradientPortal = gradient.PrepareForOutput(size, DeviceAdapter());
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  void Set(svtkm::Id index, const svtkm::Vec<T, 3>& value) const
  {
    this->GradientPortal.Set(index, value);
  }

  typename PortalTypes::Portal GradientPortal;
};

template <typename T>
struct GradientScalarOutput : public svtkm::cont::ExecutionObjectBase
{
  using ValueType = svtkm::Vec<T, 3>;
  using BaseTType = typename svtkm::VecTraits<T>::BaseComponentType;
  template <typename Device>

  SVTKM_CONT svtkm::exec::GradientScalarOutputExecutionObject<T, Device> PrepareForExecution(
    Device) const
  {
    return svtkm::exec::GradientScalarOutputExecutionObject<T, Device>(this->Gradient, this->Size);
  }

  GradientScalarOutput() = default;

  GradientScalarOutput(bool,
                       bool,
                       bool,
                       bool,
                       svtkm::cont::ArrayHandle<ValueType>& gradient,
                       svtkm::cont::ArrayHandle<BaseTType>&,
                       svtkm::cont::ArrayHandle<svtkm::Vec<BaseTType, 3>>&,
                       svtkm::cont::ArrayHandle<BaseTType>&,
                       svtkm::Id size)
    : Size(size)
    , Gradient(gradient)
  {
  }
  svtkm::Id Size;
  svtkm::cont::ArrayHandle<ValueType> Gradient;
};

template <typename T, typename DeviceAdapter>
struct GradientVecOutputExecutionObject
{
  using ValueType = svtkm::Vec<T, 3>;
  using BaseTType = typename svtkm::VecTraits<T>::BaseComponentType;

  template <typename FieldType>
  struct PortalTypes
  {
    using HandleType = svtkm::cont::ArrayHandle<FieldType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

  GradientVecOutputExecutionObject() = default;

  GradientVecOutputExecutionObject(bool g,
                                   bool d,
                                   bool v,
                                   bool q,
                                   svtkm::cont::ArrayHandle<ValueType> gradient,
                                   svtkm::cont::ArrayHandle<BaseTType> divergence,
                                   svtkm::cont::ArrayHandle<svtkm::Vec<BaseTType, 3>> vorticity,
                                   svtkm::cont::ArrayHandle<BaseTType> qcriterion,
                                   svtkm::Id size)
  {
    this->SetGradient = g;
    this->SetDivergence = d;
    this->SetVorticity = v;
    this->SetQCriterion = q;

    DeviceAdapter device;
    if (g)
    {
      this->GradientPortal = gradient.PrepareForOutput(size, device);
    }
    if (d)
    {
      this->DivergencePortal = divergence.PrepareForOutput(size, device);
    }
    if (v)
    {
      this->VorticityPortal = vorticity.PrepareForOutput(size, device);
    }
    if (q)
    {
      this->QCriterionPortal = qcriterion.PrepareForOutput(size, device);
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  void Set(svtkm::Id index, const svtkm::Vec<T, 3>& value) const
  {
    if (this->SetGradient)
    {
      this->GradientPortal.Set(index, value);
    }
    if (this->SetDivergence)
    {
      svtkm::worklet::gradient::Divergence divergence;
      BaseTType output;
      divergence(value, output);
      this->DivergencePortal.Set(index, output);
    }
    if (this->SetVorticity)
    {
      svtkm::worklet::gradient::Vorticity vorticity;
      T output;
      vorticity(value, output);
      this->VorticityPortal.Set(index, output);
    }
    if (this->SetQCriterion)
    {
      svtkm::worklet::gradient::QCriterion qc;
      BaseTType output;
      qc(value, output);
      this->QCriterionPortal.Set(index, output);
    }
  }

  bool SetGradient;
  bool SetDivergence;
  bool SetVorticity;
  bool SetQCriterion;

  typename PortalTypes<ValueType>::Portal GradientPortal;
  typename PortalTypes<BaseTType>::Portal DivergencePortal;
  typename PortalTypes<svtkm::Vec<BaseTType, 3>>::Portal VorticityPortal;
  typename PortalTypes<BaseTType>::Portal QCriterionPortal;
};

template <typename T>
struct GradientVecOutput : public svtkm::cont::ExecutionObjectBase
{
  using ValueType = svtkm::Vec<T, 3>;
  using BaseTType = typename svtkm::VecTraits<T>::BaseComponentType;

  template <typename Device>
  SVTKM_CONT svtkm::exec::GradientVecOutputExecutionObject<T, Device> PrepareForExecution(
    Device) const
  {
    return svtkm::exec::GradientVecOutputExecutionObject<T, Device>(this->G,
                                                                   this->D,
                                                                   this->V,
                                                                   this->Q,
                                                                   this->Gradient,
                                                                   this->Divergence,
                                                                   this->Vorticity,
                                                                   this->Qcriterion,
                                                                   this->Size);
  }

  GradientVecOutput() = default;

  GradientVecOutput(bool g,
                    bool d,
                    bool v,
                    bool q,
                    svtkm::cont::ArrayHandle<ValueType>& gradient,
                    svtkm::cont::ArrayHandle<BaseTType>& divergence,
                    svtkm::cont::ArrayHandle<svtkm::Vec<BaseTType, 3>>& vorticity,
                    svtkm::cont::ArrayHandle<BaseTType>& qcriterion,
                    svtkm::Id size)
  {
    this->G = g;
    this->D = d;
    this->V = v;
    this->Q = q;
    this->Gradient = gradient;
    this->Divergence = divergence;
    this->Vorticity = vorticity;
    this->Qcriterion = qcriterion;
    this->Size = size;
  }

  bool G;
  bool D;
  bool V;
  bool Q;
  svtkm::cont::ArrayHandle<ValueType> Gradient;
  svtkm::cont::ArrayHandle<BaseTType> Divergence;
  svtkm::cont::ArrayHandle<svtkm::Vec<BaseTType, 3>> Vorticity;
  svtkm::cont::ArrayHandle<BaseTType> Qcriterion;
  svtkm::Id Size;
};

template <typename T>
struct GradientOutput : public GradientScalarOutput<T>
{
  using GradientScalarOutput<T>::GradientScalarOutput;
};

template <>
struct GradientOutput<svtkm::Vec3f_32> : public GradientVecOutput<svtkm::Vec3f_32>
{
  using GradientVecOutput<svtkm::Vec3f_32>::GradientVecOutput;
};

template <>
struct GradientOutput<svtkm::Vec3f_64> : public GradientVecOutput<svtkm::Vec3f_64>
{
  using GradientVecOutput<svtkm::Vec3f_64>::GradientVecOutput;
};
}
} // namespace svtkm::exec


namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for output arrays.
///
/// \c TransportTagArrayOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for output data.
///
struct TransportTagGradientOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagGradientOut, ContObjectType, Device>
{
  using ExecObjectFacotryType = svtkm::exec::GradientOutput<typename ContObjectType::ValueType>;
  using ExecObjectType =
    decltype(std::declval<ExecObjectFacotryType>().PrepareForExecution(Device()));

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const InputDomainType& svtkmNotUsed(inputDomain),
                                      svtkm::Id svtkmNotUsed(inputRange),
                                      svtkm::Id outputRange) const
  {
    ExecObjectFacotryType ExecutionObjectFacotry = object.PrepareForOutput(outputRange);
    return ExecutionObjectFacotry.PrepareForExecution(Device());
  }
};
}
}
} // namespace svtkm::cont::arg


namespace svtkm
{
namespace worklet
{
namespace gradient
{


struct GradientOutputs : svtkm::cont::arg::ControlSignatureTagBase
{
  using TypeCheckTag = svtkm::cont::arg::TypeCheckTagExecObject;
  using TransportTag = svtkm::cont::arg::TransportTagGradientOut;
  using FetchTag = svtkm::exec::arg::FetchTagArrayDirectOut;
};
}
}
} // namespace svtkm::worklet::gradient

#endif
