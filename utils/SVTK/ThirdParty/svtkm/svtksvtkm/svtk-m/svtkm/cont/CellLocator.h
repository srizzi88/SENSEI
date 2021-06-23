//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellLocator_h
#define svtk_m_cont_CellLocator_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ExecutionObjectBase.h>

#include <svtkm/exec/CellLocator.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellLocator : public svtkm::cont::ExecutionObjectBase
{

public:
  virtual ~CellLocator();

  const svtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  void SetCellSet(const svtkm::cont::DynamicCellSet& cellSet)
  {
    this->CellSet = cellSet;
    this->SetModified();
  }

  const svtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coords; }

  void SetCoordinates(const svtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update()
  {
    if (this->Modified)
    {
      this->Build();
      this->Modified = false;
    }
  }

  SVTKM_CONT virtual const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const = 0;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  //This is going to need a TryExecute
  SVTKM_CONT virtual void Build() = 0;

private:
  svtkm::cont::DynamicCellSet CellSet;
  svtkm::cont::CoordinateSystem Coords;
  bool Modified = true;
};

} // namespace cont
} // namespace svtkm

#endif // svtk_m_cont_CellLocator_h
