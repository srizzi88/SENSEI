//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_ColorLegendAnnotation_h
#define svtk_m_rendering_ColorLegendAnnotation_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/TextAnnotationScreen.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT ColorLegendAnnotation
{
private:
  svtkm::Float32 FontScale;
  svtkm::rendering::Color LabelColor;
  std::vector<std::string> Labels;
  std::vector<std::unique_ptr<TextAnnotationScreen>> Annot;
  std::vector<svtkm::rendering::Color> ColorSwatchList;

public:
  ColorLegendAnnotation();
  ~ColorLegendAnnotation();
  ColorLegendAnnotation(const ColorLegendAnnotation&) = delete;
  ColorLegendAnnotation& operator=(const ColorLegendAnnotation&) = delete;

  void Clear();
  void AddItem(const std::string& label, svtkm::rendering::Color color);

  void SetLabelColor(svtkm::rendering::Color c) { this->LabelColor = c; }

  void SetLabelFontScale(svtkm::Float32 s)
  {
    this->FontScale = s;
    for (unsigned int i = 0; i < this->Annot.size(); i++)
      this->Annot[i]->SetScale(s);
  }

  virtual void Render(const svtkm::rendering::Camera&,
                      const svtkm::rendering::WorldAnnotator& annotator,
                      svtkm::rendering::Canvas& canvas);
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_ColorLegendAnnotation_h
