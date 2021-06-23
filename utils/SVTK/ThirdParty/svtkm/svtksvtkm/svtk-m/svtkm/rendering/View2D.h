//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_View2D_h
#define svtk_m_rendering_View2D_h

#include <svtkm/rendering/View.h>

#include <svtkm/rendering/AxisAnnotation2D.h>
#include <svtkm/rendering/ColorBarAnnotation.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT View2D : public svtkm::rendering::View
{
public:
  View2D(const svtkm::rendering::Scene& scene,
         const svtkm::rendering::Mapper& mapper,
         const svtkm::rendering::Canvas& canvas,
         const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
         const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  View2D(const svtkm::rendering::Scene& scene,
         const svtkm::rendering::Mapper& mapper,
         const svtkm::rendering::Canvas& canvas,
         const svtkm::rendering::Camera& camera,
         const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
         const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  ~View2D();

  void Paint() override;

  void RenderScreenAnnotations() override;

  void RenderWorldAnnotations() override;

private:
  void UpdateCameraProperties();

  // 2D-specific annotations
  svtkm::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  svtkm::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  svtkm::rendering::ColorBarAnnotation ColorBarAnnotation;
};
}
} // namespace svtkm::rendering

#endif //svtk_m_rendering_View2D_h
