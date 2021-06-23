//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_ColorBarAnnotation_h
#define svtk_m_rendering_ColorBarAnnotation_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/AxisAnnotation2D.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT ColorBarAnnotation
{
protected:
  svtkm::cont::ColorTable ColorTable;
  svtkm::rendering::AxisAnnotation2D Axis;
  svtkm::Bounds Position;
  bool Horizontal;
  std::string FieldName;

public:
  ColorBarAnnotation();

  virtual ~ColorBarAnnotation();

  SVTKM_CONT
  void SetColorTable(const svtkm::cont::ColorTable& colorTable) { this->ColorTable = colorTable; }

  SVTKM_CONT
  void SetRange(const svtkm::Range& range, svtkm::IdComponent numTicks);

  SVTKM_CONT
  void SetFieldName(const std::string& fieldName);

  SVTKM_CONT
  void SetRange(svtkm::Float64 l, svtkm::Float64 h, svtkm::IdComponent numTicks)
  {
    this->SetRange(svtkm::Range(l, h), numTicks);
  }


  SVTKM_CONT
  void SetPosition(const svtkm::Bounds& position);

  virtual void Render(const svtkm::rendering::Camera& camera,
                      const svtkm::rendering::WorldAnnotator& worldAnnotator,
                      svtkm::rendering::Canvas& canvas);
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_ColorBarAnnotation_h
