//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Normalize_h
#define svtk_m_worklet_Normalize_h

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace worklet
{

class Normal : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);

  template <typename T, typename T2>
  SVTKM_EXEC void operator()(const T& inValue, T2& outValue) const
  {
    outValue = svtkm::Normal(inValue);
  }
};

class Normalize : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);

  template <typename T>
  SVTKM_EXEC void operator()(T& value) const
  {
    svtkm::Normalize(value);
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_Normalize_h
