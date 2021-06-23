//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/TextAnnotation.h>
#include <svtkm/rendering/View1D.h>

namespace svtkm
{
namespace rendering
{

View1D::View1D(const svtkm::rendering::Scene& scene,
               const svtkm::rendering::Mapper& mapper,
               const svtkm::rendering::Canvas& canvas,
               const svtkm::rendering::Color& backgroundColor,
               const svtkm::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, backgroundColor, foregroundColor)
{
}

View1D::View1D(const svtkm::rendering::Scene& scene,
               const svtkm::rendering::Mapper& mapper,
               const svtkm::rendering::Canvas& canvas,
               const svtkm::rendering::Camera& camera,
               const svtkm::rendering::Color& backgroundColor,
               const svtkm::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor, foregroundColor)
{
}

View1D::~View1D()
{
}

void View1D::Paint()
{
  this->GetCanvas().Activate();
  this->GetCanvas().Clear();
  this->UpdateCameraProperties();
  this->SetupForWorldSpace();
  this->GetScene().Render(this->GetMapper(), this->GetCanvas(), this->GetCamera());
  this->RenderWorldAnnotations();
  this->SetupForScreenSpace();
  this->RenderScreenAnnotations();
  this->RenderColorLegendAnnotations();
  this->RenderAnnotations();
  this->GetCanvas().Finish();
}

void View1D::RenderScreenAnnotations()
{
  svtkm::Float32 viewportLeft;
  svtkm::Float32 viewportRight;
  svtkm::Float32 viewportTop;
  svtkm::Float32 viewportBottom;
  this->GetCamera().GetRealViewport(this->GetCanvas().GetWidth(),
                                    this->GetCanvas().GetHeight(),
                                    viewportLeft,
                                    viewportRight,
                                    viewportBottom,
                                    viewportTop);

  this->HorizontalAxisAnnotation.SetColor(AxisColor);
  this->HorizontalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportRight, viewportBottom);
  svtkm::Bounds viewRange = this->GetCamera().GetViewRange2D();

  this->HorizontalAxisAnnotation.SetLogarithmic(LogX);
  this->HorizontalAxisAnnotation.SetRangeForAutoTicks(viewRange.X.Min, viewRange.X.Max);
  this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
  this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
  this->HorizontalAxisAnnotation.SetLabelAlignment(svtkm::rendering::TextAnnotation::HCenter,
                                                   svtkm::rendering::TextAnnotation::Top);
  this->HorizontalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  svtkm::Float32 windowaspect =
    svtkm::Float32(this->GetCanvas().GetWidth()) / svtkm::Float32(this->GetCanvas().GetHeight());

  this->VerticalAxisAnnotation.SetColor(AxisColor);
  this->VerticalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportLeft, viewportTop);
  this->VerticalAxisAnnotation.SetLogarithmic(LogY);
  this->VerticalAxisAnnotation.SetRangeForAutoTicks(viewRange.Y.Min, viewRange.Y.Max);
  this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetLabelAlignment(svtkm::rendering::TextAnnotation::Right,
                                                 svtkm::rendering::TextAnnotation::VCenter);
  this->VerticalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
}

void View1D::RenderColorLegendAnnotations()
{
  if (LegendEnabled)
  {
    this->Legend.Clear();
    for (int i = 0; i < this->GetScene().GetNumberOfActors(); ++i)
    {
      svtkm::rendering::Actor act = this->GetScene().GetActor(i);

      svtkm::Vec<double, 4> colorData;
      act.GetColorTable().GetPoint(0, colorData);

      //colorData[0] is the transfer function x position
      svtkm::rendering::Color color{ static_cast<svtkm::Float32>(colorData[1]),
                                    static_cast<svtkm::Float32>(colorData[2]),
                                    static_cast<svtkm::Float32>(colorData[3]) };
      this->Legend.AddItem(act.GetScalarField().GetName(), color);
    }
    this->Legend.SetLabelColor(this->GetCanvas().GetForegroundColor());
    this->Legend.Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
  }
}

void View1D::RenderWorldAnnotations()
{
  // 1D views don't have world annotations.
}

void View1D::EnableLegend()
{
  LegendEnabled = true;
}

void View1D::DisableLegend()
{
  LegendEnabled = false;
}

void View1D::UpdateCameraProperties()
{
  // Modify the camera if we are going log scaling or if our bounds are equal
  svtkm::Bounds origCamBounds = this->GetCamera().GetViewRange2D();
  svtkm::Float64 vmin = origCamBounds.Y.Min;
  svtkm::Float64 vmax = origCamBounds.Y.Max;
  if (LogY)
  {
    if (vmin <= 0 || vmax <= 0)
    {
      origCamBounds.Y.Min = 0;
      origCamBounds.Y.Max = 1;
    }
    else
    {
      origCamBounds.Y.Min = log10(vmin);
      origCamBounds.Y.Max = log10(vmax);
      if (origCamBounds.Y.Min == origCamBounds.Y.Max)
      {
        origCamBounds.Y.Min /= 10;
        origCamBounds.Y.Max *= 10;
      }
    }
  }
  else
  {
    origCamBounds.Y.Min = vmin;
    origCamBounds.Y.Max = vmax;
    if (origCamBounds.Y.Min == origCamBounds.Y.Max)
    {
      origCamBounds.Y.Min -= .5;
      origCamBounds.Y.Max += .5;
    }
  }

  // Set camera bounds with new top/bottom values
  this->GetCamera().SetViewRange2D(
    origCamBounds.X.Min, origCamBounds.X.Max, origCamBounds.Y.Min, origCamBounds.Y.Max);

  // if unchanged by user we always want to start with a curve being full-frame
  if (this->GetCamera().GetMode() == Camera::MODE_2D && this->GetCamera().GetXScale() == 1.0f)
  {
    svtkm::Float32 left, right, bottom, top;
    this->GetCamera().GetViewRange2D(left, right, bottom, top);
    this->GetCamera().SetXScale((static_cast<svtkm::Float32>(this->GetCanvas().GetWidth())) /
                                (static_cast<svtkm::Float32>(this->GetCanvas().GetHeight())) *
                                (top - bottom) / (right - left));
  }
}
}
} // namespace svtkm::rendering
