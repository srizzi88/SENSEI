//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/Scene.h>

#include <vector>

namespace svtkm
{
namespace rendering
{

struct Scene::InternalsType
{
  std::vector<svtkm::rendering::Actor> Actors;
};

Scene::Scene()
  : Internals(new InternalsType)
{
}

void Scene::AddActor(const svtkm::rendering::Actor& actor)
{
  this->Internals->Actors.push_back(actor);
}

const svtkm::rendering::Actor& Scene::GetActor(svtkm::IdComponent index) const
{
  return this->Internals->Actors[static_cast<std::size_t>(index)];
}

svtkm::IdComponent Scene::GetNumberOfActors() const
{
  return static_cast<svtkm::IdComponent>(this->Internals->Actors.size());
}

void Scene::Render(svtkm::rendering::Mapper& mapper,
                   svtkm::rendering::Canvas& canvas,
                   const svtkm::rendering::Camera& camera) const
{
  mapper.StartScene();
  for (svtkm::IdComponent actorIndex = 0; actorIndex < this->GetNumberOfActors(); actorIndex++)
  {
    const svtkm::rendering::Actor& actor = this->GetActor(actorIndex);
    actor.Render(mapper, canvas, camera);
  }
  mapper.EndScene();
}

svtkm::Bounds Scene::GetSpatialBounds() const
{
  svtkm::Bounds bounds;
  for (svtkm::IdComponent actorIndex = 0; actorIndex < this->GetNumberOfActors(); actorIndex++)
  {
    // accumulate all Actors' spatial bounds into the scene spatial bounds
    bounds.Include(this->GetActor(actorIndex).GetSpatialBounds());
  }

  return bounds;
}
}
} // namespace svtkm::rendering
