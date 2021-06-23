//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_MapperConnectivity_h
#define svtk_m_rendering_MapperConnectivity_h

#include <svtkm/cont/ColorTable.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/View.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT MapperConnectivity : public Mapper
{
public:
  MapperConnectivity();

  ~MapperConnectivity();
  void SetSampleDistance(const svtkm::Float32&);
  void SetCanvas(svtkm::rendering::Canvas* canvas) override;
  virtual svtkm::rendering::Canvas* GetCanvas() const override;

  virtual void RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                           const svtkm::cont::CoordinateSystem& coords,
                           const svtkm::cont::Field& scalarField,
                           const svtkm::cont::ColorTable&, //colorTable
                           const svtkm::rendering::Camera& camera,
                           const svtkm::Range& scalarRange) override;

  virtual void StartScene() override;
  virtual void EndScene() override;

  svtkm::rendering::Mapper* NewCopy() const override;
  void CreateDefaultView();

protected:
  svtkm::Float32 SampleDistance;
  CanvasRayTracer* CanvasRT;
};
}
} //namespace svtkm::rendering
#endif //svtk_m_rendering_SceneRendererVolume_h
