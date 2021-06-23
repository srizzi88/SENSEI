//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_PointLocatorUniformGrid_h
#define svtk_m_cont_PointLocatorUniformGrid_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/PointLocator.h>
#include <svtkm/exec/PointLocatorUniformGrid.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT PointLocatorUniformGrid : public svtkm::cont::PointLocator
{
public:
  using RangeType = svtkm::Vec<svtkm::Range, 3>;

  void SetRange(const RangeType& range)
  {
    if (this->Range != range)
    {
      this->Range = range;
      this->SetModified();
    }
  }

  const RangeType& GetRange() const { return this->Range; }

  void SetComputeRangeFromCoordinates()
  {
    if (!this->IsRangeInvalid())
    {
      this->Range = { { 0.0, -1.0 } };
      this->SetModified();
    }
  }

  void SetNumberOfBins(const svtkm::Id3& bins)
  {
    if (this->Dims != bins)
    {
      this->Dims = bins;
      this->SetModified();
    }
  }

  const svtkm::Id3& GetNumberOfBins() const { return this->Dims; }

protected:
  void Build() override;

  struct PrepareExecutionObjectFunctor;

  SVTKM_CONT void PrepareExecutionObject(ExecutionObjectHandleType& execObjHandle,
                                        svtkm::cont::DeviceAdapterId deviceId) const override;

  bool IsRangeInvalid() const
  {
    return (this->Range[0].Max < this->Range[0].Min) || (this->Range[1].Max < this->Range[1].Min) ||
      (this->Range[2].Max < this->Range[2].Min);
  }

private:
  RangeType Range = { { 0.0, -1.0 } };
  svtkm::Id3 Dims = { 32 };

  svtkm::cont::ArrayHandle<svtkm::Id> PointIds;
  svtkm::cont::ArrayHandle<svtkm::Id> CellLower;
  svtkm::cont::ArrayHandle<svtkm::Id> CellUpper;
};
}
}
#endif //svtk_m_cont_PointLocatorUniformGrid_h
