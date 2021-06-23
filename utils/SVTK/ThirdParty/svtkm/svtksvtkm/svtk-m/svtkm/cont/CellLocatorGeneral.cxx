//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellLocatorGeneral.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellLocatorRectilinearGrid.h>
#include <svtkm/cont/CellLocatorUniformBins.h>
#include <svtkm/cont/CellLocatorUniformGrid.h>
#include <svtkm/cont/CellSetStructured.h>

namespace
{

SVTKM_CONT
void DefaultConfigurator(std::unique_ptr<svtkm::cont::CellLocator>& locator,
                         const svtkm::cont::DynamicCellSet& cellSet,
                         const svtkm::cont::CoordinateSystem& coords)
{
  using StructuredCellSet = svtkm::cont::CellSetStructured<3>;
  using UniformCoordinates = svtkm::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordinates =
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>;

  if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<UniformCoordinates>())
  {
    if (!dynamic_cast<svtkm::cont::CellLocatorUniformGrid*>(locator.get()))
    {
      locator.reset(new svtkm::cont::CellLocatorUniformGrid);
    }
  }
  else if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<RectilinearCoordinates>())
  {
    if (!dynamic_cast<svtkm::cont::CellLocatorRectilinearGrid*>(locator.get()))
    {
      locator.reset(new svtkm::cont::CellLocatorRectilinearGrid);
    }
  }
  else if (!dynamic_cast<svtkm::cont::CellLocatorUniformBins*>(locator.get()))
  {
    locator.reset(new svtkm::cont::CellLocatorUniformBins);
  }

  locator->SetCellSet(cellSet);
  locator->SetCoordinates(coords);
}

} // anonymous namespace

namespace svtkm
{
namespace cont
{

SVTKM_CONT CellLocatorGeneral::CellLocatorGeneral()
  : Configurator(DefaultConfigurator)
{
}

SVTKM_CONT CellLocatorGeneral::~CellLocatorGeneral() = default;

SVTKM_CONT const svtkm::exec::CellLocator* CellLocatorGeneral::PrepareForExecution(
  svtkm::cont::DeviceAdapterId device) const
{
  if (this->Locator)
  {
    return this->Locator->PrepareForExecution(device);
  }
  return nullptr;
}

SVTKM_CONT void CellLocatorGeneral::Build()
{
  this->Configurator(this->Locator, this->GetCellSet(), this->GetCoordinates());
  this->Locator->Update();
}

SVTKM_CONT void CellLocatorGeneral::ResetToDefaultConfigurator()
{
  this->SetConfigurator(DefaultConfigurator);
}
}
} // svtkm::cont
