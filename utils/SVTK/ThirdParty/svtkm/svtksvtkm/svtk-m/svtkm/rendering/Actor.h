//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Actor_h
#define svtk_m_rendering_Actor_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Mapper.h>

#include <memory>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT Actor
{
public:
  Actor(const svtkm::cont::DynamicCellSet& cells,
        const svtkm::cont::CoordinateSystem& coordinates,
        const svtkm::cont::Field& scalarField);

  Actor(const svtkm::cont::DynamicCellSet& cells,
        const svtkm::cont::CoordinateSystem& coordinates,
        const svtkm::cont::Field& scalarField,
        const svtkm::cont::ColorTable& colorTable);

  Actor(const svtkm::cont::DynamicCellSet& cells,
        const svtkm::cont::CoordinateSystem& coordinates,
        const svtkm::cont::Field& scalarField,
        const svtkm::rendering::Color& color);

  void Render(svtkm::rendering::Mapper& mapper,
              svtkm::rendering::Canvas& canvas,
              const svtkm::rendering::Camera& camera) const;

  const svtkm::cont::DynamicCellSet& GetCells() const;

  const svtkm::cont::CoordinateSystem& GetCoordinates() const;

  const svtkm::cont::Field& GetScalarField() const;

  const svtkm::cont::ColorTable& GetColorTable() const;

  const svtkm::Range& GetScalarRange() const;

  const svtkm::Bounds& GetSpatialBounds() const;

  void SetScalarRange(const svtkm::Range& scalarRange);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  struct RangeFunctor;

  void Init(const svtkm::cont::CoordinateSystem& coordinates, const svtkm::cont::Field& scalarField);
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_Actor_h
