//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/View2D.h>

namespace svtkm
{
namespace rendering
{

View2D::View2D(const svtkm::rendering::Scene& scene,
               const svtkm::rendering::Mapper& mapper,
               const svtkm::rendering::Canvas& canvas,
               const svtkm::rendering::Color& backgroundColor,
               const svtkm::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, backgroundColor, foregroundColor)
{
}

View2D::View2D(const svtkm::rendering::Scene& scene,
               const svtkm::rendering::Mapper& mapper,
               const svtkm::rendering::Canvas& canvas,
               const svtkm::rendering::Camera& camera,
               const svtkm::rendering::Color& backgroundColor,
               const svtkm::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor, foregroundColor)
{
}

View2D::~View2D()
{
}

void View2D::Paint()
{
  this->GetCanvas().Activate();
  this->GetCanvas().Clear();
  this->UpdateCameraProperties();
  this->SetupForWorldSpace();
  this->GetScene().Render(this->GetMapper(), this->GetCanvas(), this->GetCamera());
  this->RenderWorldAnnotations();
  this->SetupForScreenSpace();
  this->RenderScreenAnnotations();
  this->RenderAnnotations();
  this->GetCanvas().Finish();
}

void View2D::RenderScreenAnnotations()
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
  this->HorizontalAxisAnnotation.SetRangeForAutoTicks(viewRange.X.Min, viewRange.X.Max);
  this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
  this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
  this->HorizontalAxisAnnotation.SetLabelAlignment(TextAnnotation::HCenter, TextAnnotation::Top);
  this->HorizontalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  svtkm::Float32 windowaspect =
    svtkm::Float32(this->GetCanvas().GetWidth()) / svtkm::Float32(this->GetCanvas().GetHeight());

  this->VerticalAxisAnnotation.SetColor(AxisColor);
  this->VerticalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportLeft, viewportTop);
  this->VerticalAxisAnnotation.SetRangeForAutoTicks(viewRange.Y.Min, viewRange.Y.Max);
  this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetLabelAlignment(TextAnnotation::Right, TextAnnotation::VCenter);
  this->VerticalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  const svtkm::rendering::Scene& scene = this->GetScene();
  if (scene.GetNumberOfActors() > 0)
  {
    //this->ColorBarAnnotation.SetAxisColor(svtkm::rendering::Color(1,1,1));
    this->ColorBarAnnotation.SetFieldName(scene.GetActor(0).GetScalarField().GetName());
    this->ColorBarAnnotation.SetRange(
      scene.GetActor(0).GetScalarRange().Min, scene.GetActor(0).GetScalarRange().Max, 5);
    this->ColorBarAnnotation.SetColorTable(scene.GetActor(0).GetColorTable());
    this->ColorBarAnnotation.Render(
      this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
  }
}

void View2D::RenderWorldAnnotations()
{
  // 2D views don't have world annotations.
}

void View2D::UpdateCameraProperties()
{
  // Modify the camera if our bounds are equal to enable an image to show
  svtkm::Bounds origCamBounds = this->GetCamera().GetViewRange2D();
  if (origCamBounds.Y.Min == origCamBounds.Y.Max)
  {
    origCamBounds.Y.Min -= .5;
    origCamBounds.Y.Max += .5;
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
  svtkm::Float32 left, right, bottom, top;
  this->GetCamera().GetViewRange2D(left, right, bottom, top);
}
}
} // namespace svtkm::rendering
