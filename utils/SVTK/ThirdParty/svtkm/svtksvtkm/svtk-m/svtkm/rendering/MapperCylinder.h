//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_MapperCylinder_h
#define svtk_m_rendering_MapperCylinder_h

#include <svtkm/cont/ColorTable.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Mapper.h>

#include <memory>

namespace svtkm
{
namespace rendering
{

/**
 * \brief MapperCylinder renderers edges from a cell set
 *        and renders them as cylinders via ray tracing.
 *
 */
class SVTKM_RENDERING_EXPORT MapperCylinder : public Mapper
{
public:
  MapperCylinder();

  ~MapperCylinder();

  void SetCanvas(svtkm::rendering::Canvas* canvas) override;

  virtual svtkm::rendering::Canvas* GetCanvas() const override;

  /**
   * \brief render points using a variable radius based on
   *        the scalar field.
   *        The default is false.
   */
  void UseVariableRadius(bool useVariableRadius);

  /**
   * \brief Set a base radius for all points. If a
   *        radius is never specified the default heuristic
   *        is used.
   */
  void SetRadius(const svtkm::Float32& radius);

  /**
   * \brief When using a variable radius for all cylinder, the
   *        radius delta controls how much larger and smaller
   *        radii become based on the scalar field. If the delta
   *        is 0 all points will have the same radius. If the delta
   *        is 0.5 then the max/min scalar values would have a radii
   *        of base +/- base * 0.5.
   */
  void SetRadiusDelta(const svtkm::Float32& delta);

  void RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                   const svtkm::cont::CoordinateSystem& coords,
                   const svtkm::cont::Field& scalarField,
                   const svtkm::cont::ColorTable& colorTable,
                   const svtkm::rendering::Camera& camera,
                   const svtkm::Range& scalarRange) override;

  virtual void StartScene() override;
  virtual void EndScene() override;
  void SetCompositeBackground(bool on);
  svtkm::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  struct RenderFunctor;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_MapperCylinder_h
