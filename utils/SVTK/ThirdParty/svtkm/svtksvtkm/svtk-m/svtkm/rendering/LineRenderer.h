//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_rendering_LineRenderer_h
#define svtk_m_rendering_LineRenderer_h

#include <svtkm/Matrix.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/svtkm_rendering_export.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT LineRenderer
{
public:
  SVTKM_CONT
  LineRenderer(const svtkm::rendering::Canvas* canvas, svtkm::Matrix<svtkm::Float32, 4, 4> transform);

  SVTKM_CONT
  void RenderLine(const svtkm::Vec2f_64& point0,
                  const svtkm::Vec2f_64& point1,
                  svtkm::Float32 lineWidth,
                  const svtkm::rendering::Color& color);

  SVTKM_CONT
  void RenderLine(const svtkm::Vec3f_64& point0,
                  const svtkm::Vec3f_64& point1,
                  svtkm::Float32 lineWidth,
                  const svtkm::rendering::Color& color);

private:
  SVTKM_CONT
  svtkm::Vec3f_32 TransformPoint(const svtkm::Vec3f_64& point) const;

  const svtkm::rendering::Canvas* Canvas;
  svtkm::Matrix<svtkm::Float32, 4, 4> Transform;
}; // class LineRenderer
}
} // namespace svtkm::rendering

#endif // svtk_m_rendering_LineRenderer_h
