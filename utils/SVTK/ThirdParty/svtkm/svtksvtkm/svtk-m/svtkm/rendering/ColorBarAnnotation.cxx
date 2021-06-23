//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/ColorBarAnnotation.h>
#include <svtkm/rendering/TextAnnotationScreen.h>

namespace svtkm
{
namespace rendering
{

ColorBarAnnotation::ColorBarAnnotation()
  : ColorTable(svtkm::cont::ColorSpace::LAB)
  , Position(svtkm::Range(-0.88, +0.88), svtkm::Range(+0.87, +0.92), svtkm::Range(0, 0))
  , Horizontal(true)
  , FieldName("")
{
}

ColorBarAnnotation::~ColorBarAnnotation()
{
}

void ColorBarAnnotation::SetFieldName(const std::string& fieldName)
{
  FieldName = fieldName;
}

void ColorBarAnnotation::SetPosition(const svtkm::Bounds& position)
{
  Position = position;
  svtkm::Float64 x = Position.X.Length();
  svtkm::Float64 y = Position.Y.Length();
  if (x > y)
    Horizontal = true;
  else
    Horizontal = false;
}

void ColorBarAnnotation::SetRange(const svtkm::Range& range, svtkm::IdComponent numTicks)
{
  std::vector<svtkm::Float64> positions, proportions;
  this->Axis.SetMinorTicks(positions, proportions); // clear any minor ticks

  for (svtkm::IdComponent i = 0; i < numTicks; ++i)
  {
    svtkm::Float64 prop = static_cast<svtkm::Float64>(i) / static_cast<svtkm::Float64>(numTicks - 1);
    svtkm::Float64 pos = range.Min + prop * range.Length();
    positions.push_back(pos);
    proportions.push_back(prop);
  }
  this->Axis.SetMajorTicks(positions, proportions);
}

void ColorBarAnnotation::Render(const svtkm::rendering::Camera& camera,
                                const svtkm::rendering::WorldAnnotator& worldAnnotator,
                                svtkm::rendering::Canvas& canvas)
{

  canvas.AddColorBar(Position, this->ColorTable, Horizontal);

  this->Axis.SetColor(canvas.GetForegroundColor());
  this->Axis.SetLineWidth(1);

  if (Horizontal)
  {
    this->Axis.SetScreenPosition(Position.X.Min, Position.Y.Min, Position.X.Max, Position.Y.Min);
    this->Axis.SetLabelAlignment(TextAnnotation::HCenter, TextAnnotation::Top);
    this->Axis.SetMajorTickSize(0, .02, 1.0);
  }
  else
  {
    this->Axis.SetScreenPosition(Position.X.Min, Position.Y.Min, Position.X.Min, Position.Y.Max);
    this->Axis.SetLabelAlignment(TextAnnotation::Right, TextAnnotation::VCenter);
    this->Axis.SetMajorTickSize(.02, 0.0, 1.0);
  }

  this->Axis.SetMinorTickSize(0, 0, 0); // no minor ticks
  this->Axis.Render(camera, worldAnnotator, canvas);

  if (FieldName != "")
  {
    svtkm::Vec2f_32 labelPos;
    if (Horizontal)
    {
      labelPos[0] = svtkm::Float32(Position.X.Min);
      labelPos[1] = svtkm::Float32(Position.Y.Max);
    }
    else
    {
      labelPos[0] = svtkm::Float32(Position.X.Min - 0.07);
      labelPos[1] = svtkm::Float32(Position.Y.Max + 0.03);
    }

    svtkm::rendering::TextAnnotationScreen var(FieldName,
                                              canvas.GetForegroundColor(),
                                              .045f, // font scale
                                              labelPos,
                                              0.f); // rotation

    var.Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace svtkm::rendering
