//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_View1D_h
#define svtk_m_rendering_View1D_h

#include <svtkm/rendering/AxisAnnotation2D.h>
#include <svtkm/rendering/ColorLegendAnnotation.h>
#include <svtkm/rendering/View.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT View1D : public svtkm::rendering::View
{
public:
  View1D(const svtkm::rendering::Scene& scene,
         const svtkm::rendering::Mapper& mapper,
         const svtkm::rendering::Canvas& canvas,
         const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
         const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  View1D(const svtkm::rendering::Scene& scene,
         const svtkm::rendering::Mapper& mapper,
         const svtkm::rendering::Canvas& canvas,
         const svtkm::rendering::Camera& camera,
         const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
         const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  ~View1D();

  void Paint() override;
  void RenderScreenAnnotations() override;
  void RenderWorldAnnotations() override;
  void RenderColorLegendAnnotations();

  void EnableLegend();
  void DisableLegend();
  void SetLegendLabelColor(svtkm::rendering::Color c) { this->Legend.SetLabelColor(c); }

  void SetLogX(bool l)
  {
    this->GetMapper().SetLogarithmX(l);
    this->LogX = l;
  }

  void SetLogY(bool l)
  {
    this->GetMapper().SetLogarithmY(l);
    this->LogY = l;
  }

private:
  void UpdateCameraProperties();

  // 1D-specific annotations
  svtkm::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  svtkm::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  svtkm::rendering::ColorLegendAnnotation Legend;
  bool LegendEnabled = true;
  bool LogX = false;
  bool LogY = false;
};
}
} // namespace svtkm::rendering

#endif //svtk_m_rendering_View1D_h
