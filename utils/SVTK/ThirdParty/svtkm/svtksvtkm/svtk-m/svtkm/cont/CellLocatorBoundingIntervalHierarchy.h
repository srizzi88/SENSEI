//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
#define svtk_m_cont_CellLocatorBoundingIntervalHierarchy_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/VirtualObjectHandle.h>

#include <svtkm/worklet/spatialstructure/BoundingIntervalHierarchy.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellLocatorBoundingIntervalHierarchy : public svtkm::cont::CellLocator
{
public:
  SVTKM_CONT
  CellLocatorBoundingIntervalHierarchy(svtkm::IdComponent numPlanes = 4,
                                       svtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  SVTKM_CONT ~CellLocatorBoundingIntervalHierarchy() override;

  SVTKM_CONT
  void SetNumberOfSplittingPlanes(svtkm::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }

  SVTKM_CONT
  svtkm::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  SVTKM_CONT
  void SetMaxLeafSize(svtkm::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }

  SVTKM_CONT
  svtkm::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  SVTKM_CONT
  const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const override;

protected:
  SVTKM_CONT
  void Build() override;

private:
  svtkm::IdComponent NumPlanes;
  svtkm::IdComponent MaxLeafSize;
  svtkm::cont::ArrayHandle<svtkm::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  svtkm::cont::ArrayHandle<svtkm::Id> ProcessedCellIds;

  mutable svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator> ExecutionObjectHandle;
};

} // namespace cont
} // namespace svtkm

#endif // svtk_m_cont_CellLocatorBoundingIntervalHierarchy_h
