//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_PointAverage_h
#define svtk_m_worklet_PointAverage_h

#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace worklet
{

//simple functor that returns the average point value of a given
//cell based field.
class PointAverage : public svtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInCell inCellField,
                                FieldOutPoint outPointField);
  using ExecutionSignature = void(CellCount, _2, _3);
  using InputDomain = _1;

  template <typename CellValueVecType, typename OutType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& numCells,
                            const CellValueVecType& cellValues,
                            OutType& average) const
  {
    using CellValueType = typename CellValueVecType::ComponentType;
    using InVecSize =
      std::integral_constant<svtkm::IdComponent, svtkm::VecTraits<CellValueType>::NUM_COMPONENTS>;
    using OutVecSize =
      std::integral_constant<svtkm::IdComponent, svtkm::VecTraits<OutType>::NUM_COMPONENTS>;
    using SameLengthVectors = typename std::is_same<InVecSize, OutVecSize>::type;


    average = svtkm::TypeTraits<OutType>::ZeroInitialization();
    if (numCells != 0)
    {
      this->DoAverage(numCells, cellValues, average, SameLengthVectors());
    }
  }

private:
  template <typename CellValueVecType, typename OutType>
  SVTKM_EXEC void DoAverage(const svtkm::IdComponent& numCells,
                           const CellValueVecType& cellValues,
                           OutType& average,
                           std::true_type) const
  {
    using OutComponentType = typename svtkm::VecTraits<OutType>::ComponentType;
    OutType sum = OutType(cellValues[0]);
    for (svtkm::IdComponent cellIndex = 1; cellIndex < numCells; ++cellIndex)
    {
      // OutType constructor is for when OutType is a Vec.
      // static_cast is for when OutType is a small int that gets promoted to int32.
      sum = static_cast<OutType>(sum + OutType(cellValues[cellIndex]));
    }

    // OutType constructor is for when OutType is a Vec.
    // static_cast is for when OutType is a small int that gets promoted to int32.
    average = static_cast<OutType>(sum / OutType(static_cast<OutComponentType>(numCells)));
  }

  template <typename CellValueVecType, typename OutType>
  SVTKM_EXEC void DoAverage(const svtkm::IdComponent& svtkmNotUsed(numCells),
                           const CellValueVecType& svtkmNotUsed(cellValues),
                           OutType& svtkmNotUsed(average),
                           std::false_type) const
  {
    this->RaiseError("PointAverage called with mismatched Vec sizes for PointAverage.");
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_PointAverage_h
