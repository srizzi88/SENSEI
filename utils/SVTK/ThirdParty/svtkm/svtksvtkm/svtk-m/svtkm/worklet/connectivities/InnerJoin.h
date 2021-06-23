//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_connectivity_InnerJoin_h
#define svtk_m_worklet_connectivity_InnerJoin_h

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{
namespace connectivity
{
class InnerJoin
{
public:
  struct Merge : svtkm::worklet::WorkletMapField
  {
    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, WholeArrayIn, FieldOut, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, VisitIndex, _4, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    // TODO: type trait for array portal?
    template <typename KeyType, typename ValueType1, typename InPortalType, typename ValueType2>
    SVTKM_EXEC void operator()(KeyType key,
                              ValueType1 value1,
                              svtkm::Id lowerBounds,
                              svtkm::Id visitIndex,
                              const InPortalType& value2,
                              svtkm::Id& keyOut,
                              ValueType1& value1Out,
                              ValueType2& value2Out) const
    {
      auto v2 = value2.Get(lowerBounds + visitIndex);
      keyOut = key;
      value1Out = value1;
      value2Out = v2;
    }
  };

  using Algorithm = svtkm::cont::Algorithm;

  // TODO: not mutating input keys and values?
  template <typename Key, typename Value1, typename Value2>
  void Run(svtkm::cont::ArrayHandle<Key>& key1,
           svtkm::cont::ArrayHandle<Value1>& value1,
           svtkm::cont::ArrayHandle<Key>& key2,
           svtkm::cont::ArrayHandle<Value2>& value2,
           svtkm::cont::ArrayHandle<Key>& keyOut,
           svtkm::cont::ArrayHandle<Value1>& value1Out,
           svtkm::cont::ArrayHandle<Value2>& value2Out) const
  {
    Algorithm::SortByKey(key1, value1);
    Algorithm::SortByKey(key2, value2);

    svtkm::cont::ArrayHandle<svtkm::Id> lbs;
    svtkm::cont::ArrayHandle<svtkm::Id> ubs;
    Algorithm::LowerBounds(key2, key1, lbs);
    Algorithm::UpperBounds(key2, key1, ubs);

    svtkm::cont::ArrayHandle<svtkm::Id> counts;
    Algorithm::Transform(ubs, lbs, counts, svtkm::Subtract());

    svtkm::worklet::ScatterCounting scatter{ counts };
    svtkm::worklet::DispatcherMapField<Merge> mergeDisp(scatter);
    mergeDisp.Invoke(key1, value1, lbs, value2, keyOut, value1Out, value2Out);
  }
};
}
}
} // svtkm::worklet::connectivity

#endif //svtk_m_worklet_connectivity_InnerJoin_h
