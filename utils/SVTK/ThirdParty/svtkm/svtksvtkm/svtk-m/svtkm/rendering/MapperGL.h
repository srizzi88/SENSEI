//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_MapperGL_h
#define svtk_m_rendering_MapperGL_h

#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/CanvasGL.h>
#include <svtkm/rendering/Mapper.h>

#include <svtkm/rendering/internal/OpenGLHeaders.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT MapperGL : public Mapper
{
public:
  MapperGL();

  ~MapperGL();

  void RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                   const svtkm::cont::CoordinateSystem& coords,
                   const svtkm::cont::Field& scalarField,
                   const svtkm::cont::ColorTable& colorTable,
                   const svtkm::rendering::Camera&,
                   const svtkm::Range& scalarRange) override;

  void StartScene() override;
  void EndScene() override;
  void SetCanvas(svtkm::rendering::Canvas* canvas) override;
  virtual svtkm::rendering::Canvas* GetCanvas() const override;

  svtkm::rendering::Mapper* NewCopy() const override;

  svtkm::rendering::CanvasGL* Canvas;
  GLuint shader_programme;
  GLfloat mvMat[16], pMat[16];
  bool loaded;
  GLuint vao;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_MapperGL_h
