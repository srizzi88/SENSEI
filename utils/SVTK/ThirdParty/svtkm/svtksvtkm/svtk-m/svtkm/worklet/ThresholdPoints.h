//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_ThresholdPoints_h
#define svtkm_m_worklet_ThresholdPoints_h

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace worklet
{

class ThresholdPoints
{
public:
  template <typename UnaryPredicate>
  class ThresholdPointField : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutPoint passFlags);
    using ExecutionSignature = _3(_2);

    SVTKM_CONT
    ThresholdPointField()
      : Predicate()
    {
    }

    SVTKM_CONT
    explicit ThresholdPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarType>
    SVTKM_EXEC bool operator()(const ScalarType& scalar) const
    {
      return this->Predicate(scalar);
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType, typename ScalarsArrayHandle, typename UnaryPredicate>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const ScalarsArrayHandle& scalars,
                                      const UnaryPredicate& predicate)
  {
    svtkm::cont::ArrayHandle<bool> passFlags;

    using ThresholdWorklet = ThresholdPointField<UnaryPredicate>;

    ThresholdWorklet worklet(predicate);
    DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
    dispatcher.Invoke(cellSet, scalars, passFlags);

    svtkm::cont::ArrayHandle<svtkm::Id> pointIds;
    svtkm::cont::ArrayHandleCounting<svtkm::Id> indices =
      svtkm::cont::make_ArrayHandleCounting(svtkm::Id(0), svtkm::Id(1), passFlags.GetNumberOfValues());
    svtkm::cont::Algorithm::CopyIf(indices, passFlags, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    svtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(cellSet.GetNumberOfPoints(), svtkm::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_ThresholdPoints_h
