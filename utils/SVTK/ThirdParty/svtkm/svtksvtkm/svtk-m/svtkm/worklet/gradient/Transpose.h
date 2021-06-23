//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_Transpose_h
#define svtk_m_worklet_gradient_Transpose_h

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using TransposeType = svtkm::List<svtkm::Vec<svtkm::Vec<T, 3>, 3>>;

template <typename T>
struct Transpose3x3 : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut field);

  template <typename FieldInVecType>
  SVTKM_EXEC void operator()(FieldInVecType& field) const
  {
    T tempA, tempB, tempC;
    tempA = field[0][1];
    field[0][1] = field[1][0];
    field[1][0] = tempA;
    tempB = field[0][2];
    field[0][2] = field[2][0];
    field[2][0] = tempB;
    tempC = field[1][2];
    field[1][2] = field[2][1];
    field[2][1] = tempC;
  }

  template <typename S>
  void Run(svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Vec<T, 3>, 3>, S>& field,
           svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
  {
    svtkm::worklet::DispatcherMapField<Transpose3x3<T>> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(field);
  }
};
}
}
}

#endif
