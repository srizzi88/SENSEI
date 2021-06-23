//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_CellAverage_h
#define svtk_m_worklet_CellAverage_h

#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace worklet
{

//simple functor that returns the average point value as a cell field
class CellAverage : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint inPoints, FieldOutCell outCells);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  template <typename PointValueVecType, typename OutType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& numPoints,
                            const PointValueVecType& pointValues,
                            OutType& average) const
  {
    using PointValueType = typename PointValueVecType::ComponentType;

    using InVecSize =
      std::integral_constant<svtkm::IdComponent, svtkm::VecTraits<PointValueType>::NUM_COMPONENTS>;
    using OutVecSize =
      std::integral_constant<svtkm::IdComponent, svtkm::VecTraits<OutType>::NUM_COMPONENTS>;
    using SameLengthVectors = typename std::is_same<InVecSize, OutVecSize>::type;

    this->DoAverage(numPoints, pointValues, average, SameLengthVectors());
  }

private:
  template <typename PointValueVecType, typename OutType>
  SVTKM_EXEC void DoAverage(const svtkm::IdComponent& numPoints,
                           const PointValueVecType& pointValues,
                           OutType& average,
                           std::true_type) const
  {
    using OutComponentType = typename svtkm::VecTraits<OutType>::ComponentType;
    OutType sum = OutType(pointValues[0]);
    for (svtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      // OutType constructor is for when OutType is a Vec.
      // static_cast is for when OutType is a small int that gets promoted to int32.
      sum = static_cast<OutType>(sum + OutType(pointValues[pointIndex]));
    }

    // OutType constructor is for when OutType is a Vec.
    // static_cast is for when OutType is a small int that gets promoted to int32.
    average = static_cast<OutType>(sum / OutType(static_cast<OutComponentType>(numPoints)));
  }

  template <typename PointValueVecType, typename OutType>
  SVTKM_EXEC void DoAverage(const svtkm::IdComponent& svtkmNotUsed(numPoints),
                           const PointValueVecType& svtkmNotUsed(pointValues),
                           OutType& svtkmNotUsed(average),
                           std::false_type) const
  {
    this->RaiseError("CellAverage called with mismatched Vec sizes for CellAverage.");
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_CellAverage_h
