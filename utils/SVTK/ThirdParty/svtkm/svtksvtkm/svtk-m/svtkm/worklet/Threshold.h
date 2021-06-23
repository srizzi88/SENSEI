//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_Threshold_h
#define svtkm_m_worklet_Threshold_h

#include <svtkm/worklet/CellDeepCopy.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace worklet
{

class Threshold
{
public:
  enum class FieldType
  {
    Point,
    Cell
  };

  template <typename UnaryPredicate>
  class ThresholdByPointField : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutCell passFlags);

    using ExecutionSignature = _3(_2, PointCount);

    SVTKM_CONT
    ThresholdByPointField()
      : Predicate()
    {
    }

    SVTKM_CONT
    explicit ThresholdByPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarsVecType>
    SVTKM_EXEC bool operator()(const ScalarsVecType& scalars, svtkm::Id count) const
    {
      bool pass = false;
      for (svtkm::IdComponent i = 0; i < count; ++i)
      {
        pass |= this->Predicate(scalars[i]);
      }
      return pass;
    }

  private:
    UnaryPredicate Predicate;
  };

  struct ThresholdCopy : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);

    template <typename ScalarType, typename WholeFieldIn>
    SVTKM_EXEC void operator()(svtkm::Id& index,
                              ScalarType& output,
                              const WholeFieldIn& inputField) const
    {
      output = inputField.Get(index);
    }
  };


  template <typename CellSetType, typename ValueType, typename StorageType, typename UnaryPredicate>
  svtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    const svtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate)
  {
    using OutputType = svtkm::cont::CellSetPermutation<CellSetType>;

    switch (fieldType)
    {
      case svtkm::cont::Field::Association::POINTS:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;
        svtkm::cont::ArrayHandle<bool> passFlags;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);

        svtkm::cont::Algorithm::CopyIf(svtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()),
                                      passFlags,
                                      this->ValidCellIds);

        break;
      }
      case svtkm::cont::Field::Association::CELL_SET:
      {
        svtkm::cont::Algorithm::CopyIf(svtkm::cont::ArrayHandleIndex(field.GetNumberOfValues()),
                                      field,
                                      this->ValidCellIds,
                                      predicate);
        break;
      }

      default:
        throw svtkm::cont::ErrorBadValue("Expecting point or cell field.");
    }

    return OutputType(this->ValidCellIds, cellSet);
  }

  template <typename FieldArrayType, typename UnaryPredicate>
  struct CallWorklet
  {
    svtkm::cont::DynamicCellSet& Output;
    svtkm::worklet::Threshold& Worklet;
    const FieldArrayType& Field;
    const svtkm::cont::Field::Association FieldType;
    const UnaryPredicate& Predicate;

    CallWorklet(svtkm::cont::DynamicCellSet& output,
                svtkm::worklet::Threshold& worklet,
                const FieldArrayType& field,
                const svtkm::cont::Field::Association fieldType,
                const UnaryPredicate& predicate)
      : Output(output)
      , Worklet(worklet)
      , Field(field)
      , FieldType(fieldType)
      , Predicate(predicate)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellSet) const
    {
      // Copy output to an explicit grid so that other units can guess what this is.
      this->Output = svtkm::worklet::CellDeepCopy::Run(
        this->Worklet.Run(cellSet, this->Field, this->FieldType, this->Predicate));
    }
  };

  template <typename CellSetList, typename ValueType, typename StorageType, typename UnaryPredicate>
  svtkm::cont::DynamicCellSet Run(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                 const svtkm::cont::ArrayHandle<ValueType, StorageType>& field,
                                 const svtkm::cont::Field::Association fieldType,
                                 const UnaryPredicate& predicate)
  {
    using Worker = CallWorklet<svtkm::cont::ArrayHandle<ValueType, StorageType>, UnaryPredicate>;

    svtkm::cont::DynamicCellSet output;
    Worker worker(output, *this, field, fieldType, predicate);
    cellSet.CastAndCall(worker);

    return output;
  }

  template <typename ValueType, typename StorageTag>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageTag>& in) const
  {
    svtkm::cont::ArrayHandle<ValueType> result;
    DispatcherMapField<ThresholdCopy> dispatcher;
    dispatcher.Invoke(this->ValidCellIds, result, in);

    return result;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> ValidCellIds;
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_Threshold_h
