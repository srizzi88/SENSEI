//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_VolumeRendererStructured_h
#define svtk_m_rendering_raytracing_VolumeRendererStructured_h

#include <svtkm/cont/DataSet.h>

#include <svtkm/rendering/raytracing/Ray.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class VolumeRendererStructured
{
public:
  using DefaultHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using CartesianArrayHandle =
    svtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

  SVTKM_CONT
  VolumeRendererStructured();

  SVTKM_CONT
  void EnableCompositeBackground();

  SVTKM_CONT
  void DisableCompositeBackground();

  SVTKM_CONT
  void SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap);

  SVTKM_CONT
  void SetData(const svtkm::cont::CoordinateSystem& coords,
               const svtkm::cont::Field& scalarField,
               const svtkm::cont::CellSetStructured<3>& cellset,
               const svtkm::Range& scalarRange);


  SVTKM_CONT
  void Render(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays);
  //SVTKM_CONT
  ///void Render(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays);


  SVTKM_CONT
  void SetSampleDistance(const svtkm::Float32& distance);

protected:
  template <typename Precision, typename Device>
  SVTKM_CONT void RenderOnDevice(svtkm::rendering::raytracing::Ray<Precision>& rays, Device);
  template <typename Precision>
  struct RenderFunctor;

  bool IsSceneDirty;
  bool IsUniformDataSet;
  svtkm::Bounds SpatialExtent;
  svtkm::cont::ArrayHandleVirtualCoordinates Coordinates;
  svtkm::cont::CellSetStructured<3> Cellset;
  const svtkm::cont::Field* ScalarField;
  svtkm::cont::ArrayHandle<svtkm::Vec4f_32> ColorMap;
  svtkm::Float32 SampleDistance;
  svtkm::Range ScalarRange;
};
}
}
} //namespace svtkm::rendering::raytracing
#endif
