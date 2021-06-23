//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_View_h
#define svtk_m_rendering_View_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/TextAnnotation.h>

#include <memory>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT View
{
  struct InternalData;

public:
  View(const svtkm::rendering::Scene& scene,
       const svtkm::rendering::Mapper& mapper,
       const svtkm::rendering::Canvas& canvas,
       const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
       const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  View(const svtkm::rendering::Scene& scene,
       const svtkm::rendering::Mapper& mapper,
       const svtkm::rendering::Canvas& canvas,
       const svtkm::rendering::Camera& camera,
       const svtkm::rendering::Color& backgroundColor = svtkm::rendering::Color(0, 0, 0, 1),
       const svtkm::rendering::Color& foregroundColor = svtkm::rendering::Color(1, 1, 1, 1));

  virtual ~View();

  SVTKM_CONT
  const svtkm::rendering::Scene& GetScene() const;
  SVTKM_CONT
  svtkm::rendering::Scene& GetScene();
  SVTKM_CONT
  void SetScene(const svtkm::rendering::Scene& scene);

  SVTKM_CONT
  const svtkm::rendering::Mapper& GetMapper() const;
  SVTKM_CONT
  svtkm::rendering::Mapper& GetMapper();

  SVTKM_CONT
  const svtkm::rendering::Canvas& GetCanvas() const;
  SVTKM_CONT
  svtkm::rendering::Canvas& GetCanvas();

  SVTKM_CONT
  const svtkm::rendering::WorldAnnotator& GetWorldAnnotator() const;

  SVTKM_CONT
  const svtkm::rendering::Camera& GetCamera() const;
  SVTKM_CONT
  svtkm::rendering::Camera& GetCamera();
  SVTKM_CONT
  void SetCamera(const svtkm::rendering::Camera& camera);

  SVTKM_CONT
  const svtkm::rendering::Color& GetBackgroundColor() const;

  SVTKM_CONT
  void SetBackgroundColor(const svtkm::rendering::Color& color);

  SVTKM_CONT
  void SetForegroundColor(const svtkm::rendering::Color& color);

  virtual void Initialize();

  virtual void Paint() = 0;
  virtual void RenderScreenAnnotations() = 0;
  virtual void RenderWorldAnnotations() = 0;

  void SaveAs(const std::string& fileName) const;

  SVTKM_CONT
  void SetAxisColor(svtkm::rendering::Color c);

  SVTKM_CONT
  void ClearAnnotations();

  SVTKM_CONT
  void AddAnnotation(std::unique_ptr<svtkm::rendering::TextAnnotation> ann);

protected:
  void SetupForWorldSpace(bool viewportClip = true);

  void SetupForScreenSpace(bool viewportClip = false);

  void RenderAnnotations();

  svtkm::rendering::Color AxisColor = svtkm::rendering::Color::white;

private:
  std::shared_ptr<InternalData> Internal;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_View_h
