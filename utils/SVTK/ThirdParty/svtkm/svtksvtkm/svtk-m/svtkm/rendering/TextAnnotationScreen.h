//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_TextAnnotationScreen_h
#define svtk_m_rendering_TextAnnotationScreen_h

#include <svtkm/rendering/TextAnnotation.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT TextAnnotationScreen : public TextAnnotation
{
protected:
  svtkm::Vec2f_32 Position;
  svtkm::Float32 Angle;

public:
  TextAnnotationScreen(const std::string& text,
                       const svtkm::rendering::Color& color,
                       svtkm::Float32 scale,
                       const svtkm::Vec2f_32& position,
                       svtkm::Float32 angleDegrees = 0);

  ~TextAnnotationScreen();

  void SetPosition(const svtkm::Vec2f_32& position);

  void SetPosition(svtkm::Float32 posx, svtkm::Float32 posy);

  void Render(const svtkm::rendering::Camera& camera,
              const svtkm::rendering::WorldAnnotator& annotator,
              svtkm::rendering::Canvas& canvas) const override;
};
}
} // namespace svtkm::rendering

#endif //svtk_m_rendering_TextAnnotationScreen_h
