//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Dispatcher_Streaming_MapField_h
#define svtk_m_worklet_Dispatcher_Streaming_MapField_h

#include <svtkm/cont/ArrayHandleStreaming.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/internal/DispatcherBase.h>

namespace svtkm
{
namespace worklet
{

namespace detail
{

struct DispatcherStreamingTryExecuteFunctor
{
  template <typename Device, typename DispatcherBaseType, typename Invocation, typename RangeType>
  SVTKM_CONT bool operator()(Device device,
                            const DispatcherBaseType* self,
                            Invocation& invocation,
                            const RangeType& dimensions,
                            const RangeType& globalIndexOffset)
  {
    self->InvokeTransportParameters(
      invocation, dimensions, globalIndexOffset, self->Scatter.GetOutputRange(dimensions), device);
    return true;
  }
};

template <typename ControlInterface>
struct DispatcherStreamingMapFieldTransformFunctor
{
  svtkm::Id BlockIndex;
  svtkm::Id BlockSize;
  svtkm::Id CurBlockSize;
  svtkm::Id FullSize;

  SVTKM_CONT
  DispatcherStreamingMapFieldTransformFunctor(svtkm::Id blockIndex,
                                              svtkm::Id blockSize,
                                              svtkm::Id curBlockSize,
                                              svtkm::Id fullSize)
    : BlockIndex(blockIndex)
    , BlockSize(blockSize)
    , CurBlockSize(curBlockSize)
    , FullSize(fullSize)
  {
  }

  template <typename ParameterType, bool IsArrayHandle>
  struct DetermineReturnType;

  template <typename ArrayHandleType>
  struct DetermineReturnType<ArrayHandleType, true>
  {
    using type = svtkm::cont::ArrayHandleStreaming<ArrayHandleType>;
  };

  template <typename NotArrayHandleType>
  struct DetermineReturnType<NotArrayHandleType, false>
  {
    using type = NotArrayHandleType;
  };

  template <typename ParameterType, svtkm::IdComponent Index>
  struct ReturnType
  {
    using type = typename DetermineReturnType<
      ParameterType,
      svtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>::type;
  };

  template <typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template <typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    SVTKM_CONT
    svtkm::cont::ArrayHandleStreaming<ArrayHandleType> operator()(const ArrayHandleType& array,
                                                                 svtkm::Id blockIndex,
                                                                 svtkm::Id blockSize,
                                                                 svtkm::Id curBlockSize,
                                                                 svtkm::Id fullSize) const
    {
      svtkm::cont::ArrayHandleStreaming<ArrayHandleType> result =
        svtkm::cont::ArrayHandleStreaming<ArrayHandleType>(
          array, blockIndex, blockSize, curBlockSize);
      if (blockIndex == 0)
        result.AllocateFullArray(fullSize);
      return result;
    }
  };

  template <typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    SVTKM_CONT
    NotArrayHandleType operator()(const NotArrayHandleType& notArray) const { return notArray; }
  };

  template <typename ParameterType, svtkm::IdComponent Index>
  SVTKM_CONT typename ReturnType<ParameterType, Index>::type operator()(
    const ParameterType& invokeData,
    svtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType,
                         svtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()(
      invokeData, this->BlockIndex, this->BlockSize, this->CurBlockSize, this->FullSize);
  }
};

template <typename ControlInterface>
struct DispatcherStreamingMapFieldTransferFunctor
{
  SVTKM_CONT
  DispatcherStreamingMapFieldTransferFunctor() {}

  template <typename ParameterType, svtkm::IdComponent Index>
  struct ReturnType
  {
    using type = ParameterType;
  };

  template <typename ParameterType, bool IsArrayHandle>
  struct TransformImpl;

  template <typename ArrayHandleType>
  struct TransformImpl<ArrayHandleType, true>
  {
    SVTKM_CONT
    ArrayHandleType operator()(const ArrayHandleType& array) const
    {
      array.SyncControlArray();
      return array;
    }
  };

  template <typename NotArrayHandleType>
  struct TransformImpl<NotArrayHandleType, false>
  {
    SVTKM_CONT
    NotArrayHandleType operator()(const NotArrayHandleType& notArray) const { return notArray; }
  };

  template <typename ParameterType, svtkm::IdComponent Index>
  SVTKM_CONT typename ReturnType<ParameterType, Index>::type operator()(
    const ParameterType& invokeData,
    svtkm::internal::IndexTag<Index>) const
  {
    return TransformImpl<ParameterType,
                         svtkm::cont::internal::ArrayHandleCheck<ParameterType>::type::value>()(
      invokeData);
  }
};
}

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType>
class DispatcherStreamingMapField
  : public svtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType>,
                                                   WorkletType,
                                                   svtkm::worklet::WorkletMapField>
{
  using Superclass =
    svtkm::worklet::internal::DispatcherBase<DispatcherStreamingMapField<WorkletType>,
                                            WorkletType,
                                            svtkm::worklet::WorkletMapField>;
  using ScatterType = typename Superclass::ScatterType;
  using MaskType = typename WorkletType::MaskType;

public:
  template <typename... T>
  SVTKM_CONT DispatcherStreamingMapField(T&&... args)
    : Superclass(std::forward<T>(args)...)
    , NumberOfBlocks(1)
  {
  }

  SVTKM_CONT
  void SetNumberOfBlocks(svtkm::Id numberOfBlocks) { NumberOfBlocks = numberOfBlocks; }

  friend struct detail::DispatcherStreamingTryExecuteFunctor;

  template <typename Invocation>
  SVTKM_CONT void BasicInvoke(Invocation& invocation,
                             svtkm::Id numInstances,
                             svtkm::Id globalIndexOffset) const
  {
    bool success = svtkm::cont::TryExecuteOnDevice(this->GetDevice(),
                                                  detail::DispatcherStreamingTryExecuteFunctor(),
                                                  this,
                                                  invocation,
                                                  numInstances,
                                                  globalIndexOffset);
    if (!success)
    {
      throw svtkm::cont::ErrorExecution("Failed to execute worklet on any device.");
    }
  }

  template <typename Invocation>
  SVTKM_CONT void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherStreamingMapField, the inputDomain must be an ArrayHandle (or
    // an VariantArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    svtkm::Id fullSize = internal::scheduling_range(inputDomain);
    svtkm::Id blockSize = fullSize / NumberOfBlocks;
    if (fullSize % NumberOfBlocks != 0)
      blockSize += 1;

    using TransformFunctorType =
      detail::DispatcherStreamingMapFieldTransformFunctor<typename Invocation::ControlInterface>;
    using TransferFunctorType =
      detail::DispatcherStreamingMapFieldTransferFunctor<typename Invocation::ControlInterface>;

    for (svtkm::Id block = 0; block < NumberOfBlocks; block++)
    {
      // Account for domain sizes not evenly divisable by the number of blocks
      svtkm::Id numberOfInstances = blockSize;
      if (block == NumberOfBlocks - 1)
        numberOfInstances = fullSize - blockSize * block;
      svtkm::Id globalIndexOffset = blockSize * block;

      using ParameterInterfaceType = typename Invocation::ParameterInterface;
      using ReportedType =
        typename ParameterInterfaceType::template StaticTransformType<TransformFunctorType>::type;
      ReportedType newParams = invocation.Parameters.StaticTransformCont(
        TransformFunctorType(block, blockSize, numberOfInstances, fullSize));

      using ChangedType = typename Invocation::template ChangeParametersType<ReportedType>::type;
      ChangedType changedParams = invocation.ChangeParameters(newParams);

      this->BasicInvoke(changedParams, numberOfInstances, globalIndexOffset);

      // Loop over parameters again to sync results for this block into control array
      using ParameterInterfaceType2 = typename ChangedType::ParameterInterface;
      ParameterInterfaceType2& parameters2 = changedParams.Parameters;
      parameters2.StaticTransformCont(TransferFunctorType());
    }
  }

private:
  template <typename Invocation,
            typename InputRangeType,
            typename OutputRangeType,
            typename DeviceAdapter>
  SVTKM_CONT void InvokeTransportParameters(Invocation& invocation,
                                           const InputRangeType& inputRange,
                                           const InputRangeType& globalIndexOffset,
                                           const OutputRangeType& outputRange,
                                           DeviceAdapter device) const
  {
    using ParameterInterfaceType = typename Invocation::ParameterInterface;
    ParameterInterfaceType& parameters = invocation.Parameters;

    using TransportFunctorType = svtkm::worklet::internal::detail::DispatcherBaseTransportFunctor<
      typename Invocation::ControlInterface,
      typename Invocation::InputDomainType,
      DeviceAdapter>;
    using ExecObjectParameters =
      typename ParameterInterfaceType::template StaticTransformType<TransportFunctorType>::type;

    ExecObjectParameters execObjectParameters = parameters.StaticTransformCont(
      TransportFunctorType(invocation.GetInputDomain(), inputRange, outputRange));

    // Get the arrays used for scattering input to output.
    typename ScatterType::OutputToInputMapType outputToInputMap =
      this->Scatter.GetOutputToInputMap(inputRange);
    typename ScatterType::VisitArrayType visitArray = this->Scatter.GetVisitArray(inputRange);

    // Get the arrays used for masking output elements.
    typename MaskType::ThreadToOutputMapType threadToOutputMap =
      this->Mask.GetThreadToOutputMap(inputRange);

    // Replace the parameters in the invocation with the execution object and
    // pass to next step of Invoke. Also add the scatter information.
    this->InvokeSchedule(invocation.ChangeParameters(execObjectParameters)
                           .ChangeOutputToInputMap(outputToInputMap.PrepareForInput(device))
                           .ChangeVisitArray(visitArray.PrepareForInput(device))
                           .ChangeThreadToOutputMap(threadToOutputMap.PrepareForInput(device)),
                         outputRange,
                         globalIndexOffset,
                         device);
  }

  template <typename Invocation, typename RangeType, typename DeviceAdapter>
  SVTKM_CONT void InvokeSchedule(const Invocation& invocation,
                                RangeType range,
                                RangeType globalIndexOffset,
                                DeviceAdapter) const
  {
    using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;

    // The TaskType class handles the magic of fetching values
    // for each instance and calling the worklet's function.
    // The TaskType will evaluate to one of the following classes:
    //
    // svtkm::exec::internal::TaskSingular
    // svtkm::exec::internal::TaskTiling1D
    // svtkm::exec::internal::TaskTiling3D
    auto task = TaskTypes::MakeTask(this->Worklet, invocation, range, globalIndexOffset);
    Algorithm::ScheduleTask(task, range);
  }

  svtkm::Id NumberOfBlocks;
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_Dispatcher_Streaming_MapField_h
