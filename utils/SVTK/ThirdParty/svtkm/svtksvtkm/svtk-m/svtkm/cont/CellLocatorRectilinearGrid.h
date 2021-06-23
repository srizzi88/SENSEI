//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_cont_CellLocatorRectilinearGrid_h
#define svtkm_cont_CellLocatorRectilinearGrid_h

#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/VirtualObjectHandle.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellLocatorRectilinearGrid : public svtkm::cont::CellLocator
{
public:
  SVTKM_CONT CellLocatorRectilinearGrid();

  SVTKM_CONT ~CellLocatorRectilinearGrid() override;

  SVTKM_CONT const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const override;

protected:
  SVTKM_CONT void Build() override;

private:
  svtkm::Bounds Bounds;
  svtkm::Id PlaneSize;
  svtkm::Id RowSize;
  bool Is3D = true;

  mutable svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator> ExecutionObjectHandle;
};

} //namespace cont
} //namespace svtkm

#endif //svtkm_cont_CellLocatorRectilinearGrid_h
