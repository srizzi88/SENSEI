//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_Mapper_h
#define svtk_m_rendering_Mapper_h

#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT Mapper
{
public:
  SVTKM_CONT
  Mapper() {}

  virtual ~Mapper();

  virtual void RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                           const svtkm::cont::CoordinateSystem& coords,
                           const svtkm::cont::Field& scalarField,
                           const svtkm::cont::ColorTable& colorTable,
                           const svtkm::rendering::Camera& camera,
                           const svtkm::Range& scalarRange) = 0;

  virtual void SetActiveColorTable(const svtkm::cont::ColorTable& ct);

  virtual void StartScene() = 0;
  virtual void EndScene() = 0;
  virtual void SetCanvas(svtkm::rendering::Canvas* canvas) = 0;
  virtual svtkm::rendering::Canvas* GetCanvas() const = 0;

  virtual svtkm::rendering::Mapper* NewCopy() const = 0;

  virtual void SetLogarithmX(bool l);
  virtual void SetLogarithmY(bool l);

protected:
  svtkm::cont::ArrayHandle<svtkm::Vec4f_32> ColorMap;
  bool LogarithmX = false;
  bool LogarithmY = false;
};
}
} //namespace svtkm::rendering
#endif //svtk_m_rendering_Mapper_h
