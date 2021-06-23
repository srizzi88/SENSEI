//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particleadvection_GridEvaluatorStatus_h
#define svtk_m_worklet_particleadvection_GridEvaluatorStatus_h

#include <svtkm/Bitset.h>
#include <svtkm/Types.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/CellLocatorRectilinearGrid.h>
#include <svtkm/cont/CellLocatorUniformBins.h>
#include <svtkm/cont/CellLocatorUniformGrid.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <svtkm/worklet/particleadvection/Integrators.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{

class GridEvaluatorStatus : public svtkm::Bitset<svtkm::UInt8>
{
public:
  SVTKM_EXEC_CONT GridEvaluatorStatus(){};
  SVTKM_EXEC_CONT GridEvaluatorStatus(bool ok, bool spatial, bool temporal)
  {
    this->set(this->SUCCESS_BIT, ok);
    this->set(this->SPATIAL_BOUNDS_BIT, spatial);
    this->set(this->TEMPORAL_BOUNDS_BIT, temporal);
  };
  SVTKM_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  SVTKM_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  SVTKM_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  SVTKM_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  SVTKM_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT bool CheckSpatialBounds() const { return this->test(this->SPATIAL_BOUNDS_BIT); }

  SVTKM_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  SVTKM_EXEC_CONT bool CheckTemporalBounds() const { return this->test(this->TEMPORAL_BOUNDS_BIT); }

private:
  static constexpr svtkm::Id SUCCESS_BIT = 0;
  static constexpr svtkm::Id SPATIAL_BOUNDS_BIT = 1;
  static constexpr svtkm::Id TEMPORAL_BOUNDS_BIT = 2;
};
}
}
}

#endif // svtk_m_worklet_particleadvection_GridEvaluatorStatus_h
