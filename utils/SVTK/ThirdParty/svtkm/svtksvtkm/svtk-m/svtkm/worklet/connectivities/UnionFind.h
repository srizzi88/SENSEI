//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_connectivity_union_find_h
#define svtk_m_worklet_connectivity_union_find_h

#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{
namespace connectivity
{

class PointerJumping : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut comp);
  using ExecutionSignature = void(WorkIndex, _1);
  using InputDomain = _1;

  template <typename Comp>
  SVTKM_EXEC svtkm::Id findRoot(Comp& comp, svtkm::Id index) const
  {
    while (comp.Get(index) != index)
      index = comp.Get(index);
    return index;
  }

  template <typename InOutPortalType>
  SVTKM_EXEC void operator()(svtkm::Id index, InOutPortalType& comp) const
  {
    // TODO: is there a data race between findRoot and comp.Set?
    auto root = findRoot(comp, index);
    comp.Set(index, root);
  }
};

class IsStar : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn comp, AtomicArrayInOut);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  using InputDomain = _1;

  template <typename InOutPortalType, typename AtomicInOut>
  SVTKM_EXEC void operator()(svtkm::Id index, InOutPortalType& comp, AtomicInOut& hasStar) const
  {
    //hasStar emulates a LogicalAnd across all the values
    //where we start with a value of 'true'|1.
    const bool isAStar = (comp.Get(index) == comp.Get(comp.Get(index)));
    if (!isAStar && hasStar.Get(0) == 1)
    {
      hasStar.Set(0, 0);
    }
  }
};

} // connectivity
} // worklet
} // svtkm
#endif // svtk_m_worklet_connectivity_union_find_h
