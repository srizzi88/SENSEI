//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/View.h>

namespace svtkm
{
namespace rendering
{

struct View::InternalData
{
  ~InternalData()
  {
    delete MapperPointer;
    delete CanvasPointer;
    delete WorldAnnotatorPointer;
  }
  svtkm::rendering::Scene Scene;
  svtkm::rendering::Mapper* MapperPointer{ nullptr };
  svtkm::rendering::Canvas* CanvasPointer{ nullptr };
  svtkm::rendering::WorldAnnotator* WorldAnnotatorPointer{ nullptr };
  std::vector<std::unique_ptr<svtkm::rendering::TextAnnotation>> Annotations;
  svtkm::rendering::Camera Camera;
};

View::View(const svtkm::rendering::Scene& scene,
           const svtkm::rendering::Mapper& mapper,
           const svtkm::rendering::Canvas& canvas,
           const svtkm::rendering::Color& backgroundColor,
           const svtkm::rendering::Color& foregroundColor)
  : Internal(std::make_shared<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;

  svtkm::Bounds spatialBounds = this->Internal->Scene.GetSpatialBounds();
  this->Internal->Camera.ResetToBounds(spatialBounds);
  if (spatialBounds.Z.Length() > 0.0)
  {
    this->Internal->Camera.SetModeTo3D();
  }
  else
  {
    this->Internal->Camera.SetModeTo2D();
  }
}

View::View(const svtkm::rendering::Scene& scene,
           const svtkm::rendering::Mapper& mapper,
           const svtkm::rendering::Canvas& canvas,
           const svtkm::rendering::Camera& camera,
           const svtkm::rendering::Color& backgroundColor,
           const svtkm::rendering::Color& foregroundColor)
  : Internal(std::make_shared<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->Camera = camera;
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;
}

View::~View()
{
}

const svtkm::rendering::Scene& View::GetScene() const
{
  return this->Internal->Scene;
}

svtkm::rendering::Scene& View::GetScene()
{
  return this->Internal->Scene;
}

void View::SetScene(const svtkm::rendering::Scene& scene)
{
  this->Internal->Scene = scene;
}

const svtkm::rendering::Mapper& View::GetMapper() const
{
  return *this->Internal->MapperPointer;
}

svtkm::rendering::Mapper& View::GetMapper()
{
  return *this->Internal->MapperPointer;
}

const svtkm::rendering::Canvas& View::GetCanvas() const
{
  return *this->Internal->CanvasPointer;
}

svtkm::rendering::Canvas& View::GetCanvas()
{
  return *this->Internal->CanvasPointer;
}

const svtkm::rendering::WorldAnnotator& View::GetWorldAnnotator() const
{
  return *this->Internal->WorldAnnotatorPointer;
}

const svtkm::rendering::Camera& View::GetCamera() const
{
  return this->Internal->Camera;
}

svtkm::rendering::Camera& View::GetCamera()
{
  return this->Internal->Camera;
}

void View::SetCamera(const svtkm::rendering::Camera& camera)
{
  this->Internal->Camera = camera;
}

const svtkm::rendering::Color& View::GetBackgroundColor() const
{
  return this->Internal->CanvasPointer->GetBackgroundColor();
}

void View::SetBackgroundColor(const svtkm::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetBackgroundColor(color);
}

void View::SetForegroundColor(const svtkm::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetForegroundColor(color);
}

void View::Initialize()
{
  this->GetCanvas().Initialize();
}

void View::SaveAs(const std::string& fileName) const
{
  this->GetCanvas().SaveAs(fileName);
}

void View::SetAxisColor(svtkm::rendering::Color c)
{
  this->AxisColor = c;
}

void View::ClearAnnotations()
{
  this->Internal->Annotations.clear();
}

void View::AddAnnotation(std::unique_ptr<svtkm::rendering::TextAnnotation> ann)
{
  this->Internal->Annotations.push_back(std::move(ann));
}

void View::RenderAnnotations()
{
  for (unsigned int i = 0; i < this->Internal->Annotations.size(); ++i)
    this->Internal->Annotations[i]->Render(
      this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
}

void View::SetupForWorldSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToWorldSpace(this->Internal->Camera, viewportClip);
}

void View::SetupForScreenSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToScreenSpace(this->Internal->Camera, viewportClip);
}
}
} // namespace svtkm::rendering
