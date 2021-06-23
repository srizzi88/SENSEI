//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_CrossProduct_h
#define svtk_m_worklet_CrossProduct_h

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace worklet
{

class CrossProduct : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, 3>& vec1,
                            const svtkm::Vec<T, 3>& vec2,
                            svtkm::Vec<T, 3>& outVec) const
  {
    outVec = svtkm::Cross(vec1, vec2);
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_CrossProduct_h
