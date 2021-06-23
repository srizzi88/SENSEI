//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_BoundingBoxAnnotation_h
#define svtk_m_rendering_BoundingBoxAnnotation_h

#include <svtkm/Bounds.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT BoundingBoxAnnotation
{
private:
  svtkm::rendering::Color Color;
  svtkm::Bounds Extents;

public:
  BoundingBoxAnnotation();

  virtual ~BoundingBoxAnnotation();

  SVTKM_CONT
  const svtkm::Bounds& GetExtents() const { return this->Extents; }

  SVTKM_CONT
  void SetExtents(const svtkm::Bounds& extents) { this->Extents = extents; }

  SVTKM_CONT
  const svtkm::rendering::Color& GetColor() const { return this->Color; }

  SVTKM_CONT
  void SetColor(svtkm::rendering::Color c) { this->Color = c; }

  virtual void Render(const svtkm::rendering::Camera&, const WorldAnnotator& annotator);
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_BoundingBoxAnnotation_h
