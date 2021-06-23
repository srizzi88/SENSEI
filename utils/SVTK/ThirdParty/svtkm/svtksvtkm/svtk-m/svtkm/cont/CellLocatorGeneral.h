//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellLocatorGeneral_h
#define svtk_m_cont_CellLocatorGeneral_h

#include <svtkm/cont/CellLocator.h>

#include <functional>
#include <memory>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellLocatorGeneral : public svtkm::cont::CellLocator
{
public:
  SVTKM_CONT CellLocatorGeneral();

  SVTKM_CONT ~CellLocatorGeneral() override;

  /// Get the current underlying cell locator
  ///
  SVTKM_CONT const svtkm::cont::CellLocator* GetCurrentLocator() const { return this->Locator.get(); }

  /// A configurator can be provided to select an appropriate
  /// cell locator implementation and configure its parameters, based on the
  /// input cell set and cooridinates.
  /// If unset, a resonable default is used.
  ///
  using ConfiguratorSignature = void(std::unique_ptr<svtkm::cont::CellLocator>&,
                                     const svtkm::cont::DynamicCellSet&,
                                     const svtkm::cont::CoordinateSystem&);

  SVTKM_CONT void SetConfigurator(const std::function<ConfiguratorSignature>& configurator)
  {
    this->Configurator = configurator;
  }

  SVTKM_CONT const std::function<ConfiguratorSignature>& GetConfigurator() const
  {
    return this->Configurator;
  }

  SVTKM_CONT void ResetToDefaultConfigurator();

  SVTKM_CONT const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const override;

protected:
  SVTKM_CONT void Build() override;

private:
  std::unique_ptr<svtkm::cont::CellLocator> Locator;
  std::function<ConfiguratorSignature> Configurator;
};
}
} // svtkm::cont

#endif // svtk_m_cont_CellLocatorGeneral_h
