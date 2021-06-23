//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_AxisAnnotation3D_h
#define svtk_m_rendering_AxisAnnotation3D_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Range.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/AxisAnnotation.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/TextAnnotationBillboard.h>
#include <svtkm/rendering/WorldAnnotator.h>

#include <sstream>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT AxisAnnotation3D : public AxisAnnotation
{
private:
protected:
  svtkm::Float64 TickMajorSize, TickMajorOffset;
  svtkm::Float64 TickMinorSize, TickMinorOffset;
  int Axis;
  svtkm::Vec3f_32 Invert;
  svtkm::Vec3f_64 Point0, Point1;
  svtkm::Range Range;
  svtkm::Float64 FontScale;
  svtkm::Float32 FontOffset;
  svtkm::Float32 LineWidth;
  svtkm::rendering::Color Color;
  std::vector<std::unique_ptr<TextAnnotationBillboard>> Labels;
  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation3D();

  ~AxisAnnotation3D();

  AxisAnnotation3D(const AxisAnnotation3D&) = delete;

  AxisAnnotation3D& operator=(const AxisAnnotation3D&) = delete;

  SVTKM_CONT
  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  SVTKM_CONT
  void SetColor(svtkm::rendering::Color c) { this->Color = c; }

  SVTKM_CONT
  void SetAxis(int a) { this->Axis = a; }

  void SetTickInvert(bool x, bool y, bool z);

  /// offset of 0 means the tick is inside the frame
  /// offset of 1 means the tick is outside the frame
  /// offset of 0.5 means the tick is centered on the frame
  SVTKM_CONT
  void SetMajorTickSize(svtkm::Float64 size, svtkm::Float64 offset)
  {
    this->TickMajorSize = size;
    this->TickMajorOffset = offset;
  }
  SVTKM_CONT
  void SetMinorTickSize(svtkm::Float64 size, svtkm::Float64 offset)
  {
    this->TickMinorSize = size;
    this->TickMinorOffset = offset;
  }

  SVTKM_CONT
  void SetWorldPosition(const svtkm::Vec3f_64& point0, const svtkm::Vec3f_64& point1)
  {
    this->Point0 = point0;
    this->Point1 = point1;
  }

  SVTKM_CONT
  void SetWorldPosition(svtkm::Float64 x0,
                        svtkm::Float64 y0,
                        svtkm::Float64 z0,
                        svtkm::Float64 x1,
                        svtkm::Float64 y1,
                        svtkm::Float64 z1)
  {
    this->SetWorldPosition(svtkm::make_Vec(x0, y0, z0), svtkm::make_Vec(x1, y1, z1));
  }

  void SetLabelFontScale(svtkm::Float64 s);

  void SetLabelFontOffset(svtkm::Float32 off) { this->FontOffset = off; }

  void SetRange(const svtkm::Range& range) { this->Range = range; }

  void SetRange(svtkm::Float64 lower, svtkm::Float64 upper)
  {
    this->SetRange(svtkm::Range(lower, upper));
  }

  virtual void Render(const svtkm::rendering::Camera& camera,
                      const svtkm::rendering::WorldAnnotator& worldAnnotator,
                      svtkm::rendering::Canvas& canvas) override;
};
}
} //namespace svtkm::rendering

#endif // svtk_m_rendering_AxisAnnotation3D_h
