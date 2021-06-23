//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_cuda_internal_TaskStrided_h
#define svtk_m_exec_cuda_internal_TaskStrided_h

#include <svtkm/exec/TaskBase.h>

#include <svtkm/cont/cuda/internal/CudaAllocator.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <svtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace svtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

template <typename WType>
void TaskStridedSetErrorBuffer(void* w, const svtkm::exec::internal::ErrorMessageBuffer& buffer)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  WorkletType* const worklet = static_cast<WorkletType*>(w);
  worklet->SetErrorMessageBuffer(buffer);
}

class TaskStrided : public svtkm::exec::TaskBase
{
public:
  void SetErrorMessageBuffer(const svtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    (void)buffer;
    this->SetErrorBufferFunction(this->WPtr, buffer);
  }

protected:
  void* WPtr = nullptr;

  using SetErrorBufferSignature = void (*)(void*, const svtkm::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction = nullptr;
};

template <typename WType, typename IType>
class TaskStrided1D : public TaskStrided
{
public:
  TaskStrided1D(const WType& worklet, const IType& invocation, svtkm::Id globalIndexOffset = 0)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  SVTKM_EXEC
  void operator()(svtkm::Id start, svtkm::Id end, svtkm::Id inc) const
  {
    for (svtkm::Id index = start; index < end; index += inc)
    {
      //Todo: rename this function to DoTaskInvokeWorklet
      svtkm::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain(),
                                       this->GlobalIndexOffset));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
  const svtkm::Id GlobalIndexOffset;
};

template <typename WType>
class TaskStrided1D<WType, svtkm::internal::NullType> : public TaskStrided
{
public:
  TaskStrided1D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  SVTKM_EXEC
  void operator()(svtkm::Id start, svtkm::Id end, svtkm::Id inc) const
  {
    for (svtkm::Id index = start; index < end; index += inc)
    {
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};

template <typename WType, typename IType>
class TaskStrided3D : public TaskStrided
{
public:
  TaskStrided3D(const WType& worklet, const IType& invocation, svtkm::Id globalIndexOffset = 0)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  SVTKM_EXEC
  void operator()(svtkm::Id start, svtkm::Id end, svtkm::Id inc, svtkm::Id j, svtkm::Id k) const
  {
    svtkm::Id3 index(start, j, k);
    for (svtkm::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      //Todo: rename this function to DoTaskInvokeWorklet
      svtkm::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain(),
                                       this->GlobalIndexOffset));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
  const svtkm::Id GlobalIndexOffset;
};

template <typename WType>
class TaskStrided3D<WType, svtkm::internal::NullType> : public TaskStrided
{
public:
  TaskStrided3D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  SVTKM_EXEC
  void operator()(svtkm::Id start, svtkm::Id end, svtkm::Id inc, svtkm::Id j, svtkm::Id k) const
  {
    svtkm::Id3 index(start, j, k);
    for (svtkm::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};
}
}
}
} // svtkm::exec::cuda::internal

#endif //svtk_m_exec_cuda_internal_TaskStrided_h
