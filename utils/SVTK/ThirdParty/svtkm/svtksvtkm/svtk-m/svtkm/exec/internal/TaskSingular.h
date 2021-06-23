//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_internal_TaskSingular_h
#define svtk_m_exec_internal_TaskSingular_h

#include <svtkm/internal/Invocation.h>

#include <svtkm/exec/TaskBase.h>

#include <svtkm/exec/arg/Fetch.h>

//Todo: rename this header to TaskSingularDetail.h
#include <svtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace svtkm
{
namespace exec
{
namespace internal
{

// TaskSingular represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. Inside
// this single dimension no order is preferred.
//
//
template <typename WorkletType, typename InvocationType>
class TaskSingular : public svtkm::exec::TaskBase
{
public:
  SVTKM_CONT
  TaskSingular(const WorkletType& worklet,
               const InvocationType& invocation,
               svtkm::Id globalIndexOffset = 0)
    : Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
  }

  SVTKM_CONT
  void SetErrorMessageBuffer(const svtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC void operator()(T index) const
  {
    //Todo: rename this function to DoTaskSingular
    detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(index,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain(),
                                     GlobalIndexOffset));
  }

private:
  typename std::remove_const<WorkletType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const InvocationType Invocation;
  const svtkm::Id GlobalIndexOffset;
};
}
}
} // svtkm::exec::internal

#endif //svtk_m_exec_internal_TaskSingular_h
