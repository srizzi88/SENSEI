//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_AxisAnnotation2D_h
#define svtk_m_rendering_AxisAnnotation2D_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Range.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/AxisAnnotation.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/TextAnnotation.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT AxisAnnotation2D : public AxisAnnotation
{
protected:
  svtkm::Float64 MajorTickSizeX, MajorTickSizeY, MajorTickOffset;
  svtkm::Float64 MinorTickSizeX, MinorTickSizeY, MinorTickOffset;
  svtkm::Float64 PosX0, PosY0, PosX1, PosY1;
  svtkm::Range TickRange;
  svtkm::Float32 FontScale;
  svtkm::Float32 LineWidth;
  svtkm::rendering::Color Color;
  bool Logarithmic;

  TextAnnotation::HorizontalAlignment AlignH;
  TextAnnotation::VerticalAlignment AlignV;
  std::vector<std::unique_ptr<TextAnnotation>> Labels;
  //  std::vector<TextAnnotation*> Labels;

  std::vector<svtkm::Float64> PositionsMajor;
  std::vector<svtkm::Float64> ProportionsMajor;

  std::vector<svtkm::Float64> PositionsMinor;
  std::vector<svtkm::Float64> ProportionsMinor;

  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation2D();

  ~AxisAnnotation2D();

  AxisAnnotation2D(const AxisAnnotation2D&) = delete;

  AxisAnnotation2D& operator=(const AxisAnnotation2D&) = delete;

  void SetLogarithmic(bool l) { this->Logarithmic = l; }

  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  void SetColor(svtkm::rendering::Color c) { this->Color = c; }

  void SetLineWidth(svtkm::Float32 lw) { this->LineWidth = lw; }

  void SetMajorTickSize(svtkm::Float64 xlen, svtkm::Float64 ylen, svtkm::Float64 offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    this->MajorTickSizeX = xlen;
    this->MajorTickSizeY = ylen;
    this->MajorTickOffset = offset;
  }

  void SetMinorTickSize(svtkm::Float64 xlen, svtkm::Float64 ylen, svtkm::Float64 offset)
  {
    this->MinorTickSizeX = xlen;
    this->MinorTickSizeY = ylen;
    this->MinorTickOffset = offset;
  }

  ///\todo: rename, since it might be screen OR world position?
  void SetScreenPosition(svtkm::Float64 x0, svtkm::Float64 y0, svtkm::Float64 x1, svtkm::Float64 y1)
  {
    this->PosX0 = x0;
    this->PosY0 = y0;

    this->PosX1 = x1;
    this->PosY1 = y1;
  }

  void SetLabelAlignment(TextAnnotation::HorizontalAlignment h, TextAnnotation::VerticalAlignment v)
  {
    this->AlignH = h;
    this->AlignV = v;
  }

  void SetLabelFontScale(svtkm::Float32 s)
  {
    this->FontScale = s;
    for (unsigned int i = 0; i < this->Labels.size(); i++)
      this->Labels[i]->SetScale(s);
  }

  void SetRangeForAutoTicks(const svtkm::Range& range);
  void SetRangeForAutoTicks(svtkm::Float64 lower, svtkm::Float64 upper)
  {
    this->SetRangeForAutoTicks(svtkm::Range(lower, upper));
  }

  void SetMajorTicks(const std::vector<svtkm::Float64>& positions,
                     const std::vector<svtkm::Float64>& proportions);

  void SetMinorTicks(const std::vector<svtkm::Float64>& positions,
                     const std::vector<svtkm::Float64>& proportions);

  void Render(const svtkm::rendering::Camera& camera,
              const svtkm::rendering::WorldAnnotator& worldAnnotator,
              svtkm::rendering::Canvas& canvas) override;
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_AxisAnnotation2D_h
