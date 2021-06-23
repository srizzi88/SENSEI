//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_rendering_TextAnnotationBillboard_h
#define svtkm_rendering_TextAnnotationBillboard_h

#include <svtkm/rendering/TextAnnotation.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT TextAnnotationBillboard : public TextAnnotation
{
protected:
  svtkm::Vec3f_32 Position;
  svtkm::Float32 Angle;

public:
  TextAnnotationBillboard(const std::string& text,
                          const svtkm::rendering::Color& color,
                          svtkm::Float32 scalar,
                          const svtkm::Vec3f_32& position,
                          svtkm::Float32 angleDegrees = 0);

  ~TextAnnotationBillboard();

  void SetPosition(const svtkm::Vec3f_32& position);

  void SetPosition(svtkm::Float32 posx, svtkm::Float32 posy, svtkm::Float32 posz);

  void Render(const svtkm::rendering::Camera& camera,
              const svtkm::rendering::WorldAnnotator& worldAnnotator,
              svtkm::rendering::Canvas& canvas) const override;
};
}
} // namespace svtkm::rendering

#endif //svtkm_rendering_TextAnnotationBillboard_h
