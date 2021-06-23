//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_WorldAnnotator_h
#define svtk_m_rendering_WorldAnnotator_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Types.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>

namespace svtkm
{
namespace rendering
{

class Canvas;

class SVTKM_RENDERING_EXPORT WorldAnnotator
{
public:
  WorldAnnotator(const svtkm::rendering::Canvas* canvas);

  virtual ~WorldAnnotator();

  virtual void AddLine(const svtkm::Vec3f_64& point0,
                       const svtkm::Vec3f_64& point1,
                       svtkm::Float32 lineWidth,
                       const svtkm::rendering::Color& color,
                       bool inFront = false) const;

  SVTKM_CONT
  void AddLine(svtkm::Float64 x0,
               svtkm::Float64 y0,
               svtkm::Float64 z0,
               svtkm::Float64 x1,
               svtkm::Float64 y1,
               svtkm::Float64 z1,
               svtkm::Float32 lineWidth,
               const svtkm::rendering::Color& color,
               bool inFront = false) const
  {
    this->AddLine(
      svtkm::make_Vec(x0, y0, z0), svtkm::make_Vec(x1, y1, z1), lineWidth, color, inFront);
  }

  virtual void AddText(const svtkm::Vec3f_32& origin,
                       const svtkm::Vec3f_32& right,
                       const svtkm::Vec3f_32& up,
                       svtkm::Float32 scale,
                       const svtkm::Vec2f_32& anchor,
                       const svtkm::rendering::Color& color,
                       const std::string& text,
                       const svtkm::Float32 depth = 0.f) const;

  SVTKM_CONT
  void AddText(svtkm::Float32 originX,
               svtkm::Float32 originY,
               svtkm::Float32 originZ,
               svtkm::Float32 rightX,
               svtkm::Float32 rightY,
               svtkm::Float32 rightZ,
               svtkm::Float32 upX,
               svtkm::Float32 upY,
               svtkm::Float32 upZ,
               svtkm::Float32 scale,
               svtkm::Float32 anchorX,
               svtkm::Float32 anchorY,
               const svtkm::rendering::Color& color,
               const std::string& text) const
  {
    this->AddText(svtkm::make_Vec(originX, originY, originZ),
                  svtkm::make_Vec(rightX, rightY, rightZ),
                  svtkm::make_Vec(upX, upY, upZ),
                  scale,
                  svtkm::make_Vec(anchorX, anchorY),
                  color,
                  text);
  }

private:
  const svtkm::rendering::Canvas* Canvas;
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_WorldAnnotator_h
