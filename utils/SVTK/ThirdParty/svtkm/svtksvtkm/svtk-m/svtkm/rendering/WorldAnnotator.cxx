//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/LineRenderer.h>
#include <svtkm/rendering/TextRenderer.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

WorldAnnotator::WorldAnnotator(const svtkm::rendering::Canvas* canvas)
  : Canvas(canvas)
{
}

WorldAnnotator::~WorldAnnotator()
{
}

void WorldAnnotator::AddLine(const svtkm::Vec3f_64& point0,
                             const svtkm::Vec3f_64& point1,
                             svtkm::Float32 lineWidth,
                             const svtkm::rendering::Color& color,
                             bool svtkmNotUsed(inFront)) const
{
  svtkm::Matrix<svtkm::Float32, 4, 4> transform =
    svtkm::MatrixMultiply(Canvas->GetProjection(), Canvas->GetModelView());
  LineRenderer renderer(Canvas, transform);
  renderer.RenderLine(point0, point1, lineWidth, color);
}

void WorldAnnotator::AddText(const svtkm::Vec3f_32& origin,
                             const svtkm::Vec3f_32& right,
                             const svtkm::Vec3f_32& up,
                             svtkm::Float32 scale,
                             const svtkm::Vec2f_32& anchor,
                             const svtkm::rendering::Color& color,
                             const std::string& text,
                             const svtkm::Float32 depth) const
{
  svtkm::Vec3f_32 n = svtkm::Cross(right, up);
  svtkm::Normalize(n);

  svtkm::Matrix<svtkm::Float32, 4, 4> transform = MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = svtkm::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = svtkm::MatrixMultiply(Canvas->GetProjection(), transform);
  Canvas->AddText(transform, scale, anchor, color, text, depth);
}
}
} // namespace svtkm::rendering
