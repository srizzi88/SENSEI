//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Scene_h
#define svtk_m_rendering_Scene_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Mapper.h>

#include <memory>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT Scene
{
public:
  Scene();

  void AddActor(const svtkm::rendering::Actor& actor);

  const svtkm::rendering::Actor& GetActor(svtkm::IdComponent index) const;

  svtkm::IdComponent GetNumberOfActors() const;

  void Render(svtkm::rendering::Mapper& mapper,
              svtkm::rendering::Canvas& canvas,
              const svtkm::rendering::Camera& camera) const;

  svtkm::Bounds GetSpatialBounds() const;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_Scene_h
