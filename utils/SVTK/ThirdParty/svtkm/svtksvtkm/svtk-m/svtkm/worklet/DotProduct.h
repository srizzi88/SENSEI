//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_DotProduct_h
#define svtk_m_worklet_DotProduct_h

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace worklet
{

class DotProduct : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, svtkm::IdComponent Size>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, Size>& v1,
                            const svtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = svtkm::Dot(v1, v2);
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_Normalize_h
