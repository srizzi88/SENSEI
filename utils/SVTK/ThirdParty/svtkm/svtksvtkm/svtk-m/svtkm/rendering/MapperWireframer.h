//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_MapperWireframer_h
#define svtk_m_rendering_MapperWireframer_h

#include <memory>

#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Mapper.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT MapperWireframer : public Mapper
{
public:
  SVTKM_CONT
  MapperWireframer();
  virtual ~MapperWireframer();

  virtual svtkm::rendering::Canvas* GetCanvas() const override;
  virtual void SetCanvas(svtkm::rendering::Canvas* canvas) override;

  bool GetShowInternalZones() const;
  void SetShowInternalZones(bool showInternalZones);
  void SetCompositeBackground(bool on);

  bool GetIsOverlay() const;
  void SetIsOverlay(bool isOverlay);

  virtual void StartScene() override;
  virtual void EndScene() override;

  virtual void RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                           const svtkm::cont::CoordinateSystem& coords,
                           const svtkm::cont::Field& scalarField,
                           const svtkm::cont::ColorTable& colorTable,
                           const svtkm::rendering::Camera& camera,
                           const svtkm::Range& scalarRange) override;

  virtual svtkm::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
}; // class MapperWireframer
}
} // namespace svtkm::rendering
#endif // svtk_m_rendering_MapperWireframer_h
