//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particleadvection_TemporalGridEvaluators_h
#define svtk_m_worklet_particleadvection_TemporalGridEvaluators_h

#include <svtkm/worklet/particleadvection/GridEvaluatorStatus.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionTemporalGridEvaluator
{
private:
  using GridEvaluator = svtkm::worklet::particleadvection::GridEvaluator<FieldArrayType>;
  using ExecutionGridEvaluator =
    svtkm::worklet::particleadvection::ExecutionGridEvaluator<DeviceAdapter, FieldArrayType>;

public:
  SVTKM_CONT
  ExecutionTemporalGridEvaluator() = default;

  SVTKM_CONT
  ExecutionTemporalGridEvaluator(const GridEvaluator& evaluatorOne,
                                 const svtkm::FloatDefault timeOne,
                                 const GridEvaluator& evaluatorTwo,
                                 const svtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne.PrepareForExecution(DeviceAdapter()))
    , EvaluatorTwo(evaluatorTwo.PrepareForExecution(DeviceAdapter()))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
    , TimeDiff(timeTwo - timeOne)
  {
  }

  template <typename Point>
  SVTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    return this->EvaluatorOne.IsWithinSpatialBoundary(point) &&
      this->EvaluatorTwo.IsWithinSpatialBoundary(point);
  }

  SVTKM_EXEC
  bool IsWithinTemporalBoundary(const svtkm::FloatDefault time) const
  {
    return time >= TimeOne && time <= TimeTwo;
  }

  SVTKM_EXEC
  svtkm::Bounds GetSpatialBoundary() const { return this->EvaluatorTwo.GetSpatialBoundary(); }

  SVTKM_EXEC_CONT
  svtkm::FloatDefault GetTemporalBoundary(svtkm::Id direction) const
  {
    return direction > 0 ? this->TimeTwo : this->TimeOne;
  }

  template <typename Point>
  SVTKM_EXEC GridEvaluatorStatus Evaluate(const Point& pos,
                                         svtkm::FloatDefault time,
                                         Point& out) const
  {
    // Validate time is in bounds for the current two slices.
    GridEvaluatorStatus status;

    if (!(time >= TimeOne && time <= TimeTwo))
    {
      status.SetFail();
      status.SetTemporalBounds();
      return status;
    }

    Point one, two;
    status = this->EvaluatorOne.Evaluate(pos, one);
    if (status.CheckFail())
      return status;
    status = this->EvaluatorTwo.Evaluate(pos, two);
    if (status.CheckFail())
      return status;

    // LERP between the two values of calculated fields to obtain the new value
    svtkm::FloatDefault proportion = (time - this->TimeOne) / this->TimeDiff;
    out = svtkm::Lerp(one, two, proportion);

    status.SetOk();
    return status;
  }

private:
  ExecutionGridEvaluator EvaluatorOne;
  ExecutionGridEvaluator EvaluatorTwo;
  svtkm::FloatDefault TimeOne;
  svtkm::FloatDefault TimeTwo;
  svtkm::FloatDefault TimeDiff;
};

template <typename FieldArrayType>
class TemporalGridEvaluator : public svtkm::cont::ExecutionObjectBase
{
private:
  using GridEvaluator = svtkm::worklet::particleadvection::GridEvaluator<FieldArrayType>;

public:
  SVTKM_CONT TemporalGridEvaluator() = default;

  SVTKM_CONT TemporalGridEvaluator(GridEvaluator& evaluatorOne,
                                  const svtkm::FloatDefault timeOne,
                                  GridEvaluator& evaluatorTwo,
                                  const svtkm::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne)
    , EvaluatorTwo(evaluatorTwo)
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  SVTKM_CONT TemporalGridEvaluator(const svtkm::cont::CoordinateSystem& coordinatesOne,
                                  const svtkm::cont::DynamicCellSet& cellsetOne,
                                  const FieldArrayType& fieldOne,
                                  const svtkm::FloatDefault timeOne,
                                  const svtkm::cont::CoordinateSystem& coordinatesTwo,
                                  const svtkm::cont::DynamicCellSet& cellsetTwo,
                                  const FieldArrayType& fieldTwo,
                                  const svtkm::FloatDefault timeTwo)
    : EvaluatorOne(GridEvaluator(coordinatesOne, cellsetOne, fieldOne))
    , EvaluatorTwo(GridEvaluator(coordinatesTwo, cellsetTwo, fieldTwo))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  template <typename DeviceAdapter>
  SVTKM_CONT ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType> PrepareForExecution(
    DeviceAdapter) const
  {
    return ExecutionTemporalGridEvaluator<DeviceAdapter, FieldArrayType>(
      this->EvaluatorOne, this->TimeOne, this->EvaluatorTwo, this->TimeTwo);
  }

private:
  GridEvaluator EvaluatorOne;
  GridEvaluator EvaluatorTwo;
  svtkm::FloatDefault TimeOne;
  svtkm::FloatDefault TimeTwo;
};

} // namespace particleadvection
} // namespace worklet
} // namespace svtkm

#endif
