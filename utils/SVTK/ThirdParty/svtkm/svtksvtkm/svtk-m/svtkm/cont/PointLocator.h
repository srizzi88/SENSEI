//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_PointLocator_h
#define svtk_m_cont_PointLocator_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/cont/VirtualObjectHandle.h>
#include <svtkm/exec/PointLocator.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT PointLocator : public svtkm::cont::ExecutionObjectBase
{
public:
  virtual ~PointLocator();

  PointLocator()
    : Modified(true)
  {
  }

  svtkm::cont::CoordinateSystem GetCoordinates() const { return this->Coords; }

  void SetCoordinates(const svtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update()
  {
    if (this->Modified)
    {
      Build();
      this->Modified = false;
    }
  }

  SVTKM_CONT const svtkm::exec::PointLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const
  {
    this->PrepareExecutionObject(this->ExecutionObjectHandle, device);
    return this->ExecutionObjectHandle.PrepareForExecution(device);
  }

protected:
  void SetModified() { this->Modified = true; }

  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;

  using ExecutionObjectHandleType = svtkm::cont::VirtualObjectHandle<svtkm::exec::PointLocator>;

  SVTKM_CONT virtual void PrepareExecutionObject(ExecutionObjectHandleType& execObjHandle,
                                                svtkm::cont::DeviceAdapterId deviceId) const = 0;

private:
  svtkm::cont::CoordinateSystem Coords;
  bool Modified;

  mutable ExecutionObjectHandleType ExecutionObjectHandle;
};
}
}
#endif // svtk_m_cont_PointLocator_h
