//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_AxisAnnotation_h
#define svtk_m_rendering_AxisAnnotation_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/WorldAnnotator.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT AxisAnnotation
{
protected:
  void CalculateTicks(const svtkm::Range& range,
                      bool minor,
                      std::vector<svtkm::Float64>& positions,
                      std::vector<svtkm::Float64>& proportions,
                      int modifyTickQuantity) const;
  void CalculateTicksLogarithmic(const svtkm::Range& range,
                                 bool minor,
                                 std::vector<svtkm::Float64>& positions,
                                 std::vector<svtkm::Float64>& proportions) const;

public:
  AxisAnnotation();

  virtual ~AxisAnnotation();

  virtual void Render(const svtkm::rendering::Camera& camera,
                      const svtkm::rendering::WorldAnnotator& worldAnnotator,
                      svtkm::rendering::Canvas& canvas) = 0;
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_AxisAnnotation_h
