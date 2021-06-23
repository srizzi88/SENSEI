//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_TextAnnotation_h
#define svtk_m_rendering_TextAnnotation_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT TextAnnotation
{
public:
  enum HorizontalAlignment
  {
    Left,
    HCenter,
    Right
  };
  enum VerticalAlignment
  {
    Bottom,
    VCenter,
    Top
  };

protected:
  std::string Text;
  Color TextColor;
  svtkm::Float32 Scale;
  svtkm::Vec2f_32 Anchor;

public:
  TextAnnotation(const std::string& text,
                 const svtkm::rendering::Color& color,
                 svtkm::Float32 scalar);

  virtual ~TextAnnotation();

  void SetText(const std::string& text);

  const std::string& GetText() const;

  /// Set the anchor point relative to the box containing the text. The anchor
  /// is scaled in both directions to the range [-1,1] with -1 at the lower
  /// left and 1 at the upper right.
  ///
  void SetRawAnchor(const svtkm::Vec2f_32& anchor);

  void SetRawAnchor(svtkm::Float32 h, svtkm::Float32 v);

  void SetAlignment(HorizontalAlignment h, VerticalAlignment v);

  void SetScale(svtkm::Float32 scale);

  virtual void Render(const svtkm::rendering::Camera& camera,
                      const svtkm::rendering::WorldAnnotator& worldAnnotator,
                      svtkm::rendering::Canvas& canvas) const = 0;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_TextAnnotation_h
