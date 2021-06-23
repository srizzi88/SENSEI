//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_cont_CellLocatorUniformGrid_h
#define svtkm_cont_CellLocatorUniformGrid_h

#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/VirtualObjectHandle.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellLocatorUniformGrid : public svtkm::cont::CellLocator
{
public:
  SVTKM_CONT CellLocatorUniformGrid();

  SVTKM_CONT ~CellLocatorUniformGrid() override;

  SVTKM_CONT const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const override;

protected:
  SVTKM_CONT void Build() override;

private:
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;
  svtkm::Vec3f Origin;
  svtkm::Vec3f InvSpacing;
  svtkm::Vec3f MaxPoint;
  bool Is3D = true;

  mutable svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator> ExecutionObjectHandle;
};
}
} // svtkm::cont

#endif //svtkm_cont_CellLocatorUniformGrid_h
