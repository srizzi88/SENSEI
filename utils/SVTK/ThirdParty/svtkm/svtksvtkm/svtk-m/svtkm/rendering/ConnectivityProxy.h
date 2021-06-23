//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_ConnectivityProxy_h
#define svtk_m_rendering_ConnectivityProxy_h

#include <memory>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/View.h>
#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/PartialComposite.h>
#include <svtkm/rendering/raytracing/Ray.h>

namespace svtkm
{
namespace rendering
{

using PartialVector64 = std::vector<svtkm::rendering::raytracing::PartialComposite<svtkm::Float64>>;
using PartialVector32 = std::vector<svtkm::rendering::raytracing::PartialComposite<svtkm::Float32>>;

class SVTKM_RENDERING_EXPORT ConnectivityProxy
{
public:
  ConnectivityProxy(svtkm::cont::DataSet& dataset);
  ConnectivityProxy(const svtkm::cont::DynamicCellSet& cellset,
                    const svtkm::cont::CoordinateSystem& coords,
                    const svtkm::cont::Field& scalarField);
  ~ConnectivityProxy();
  enum RenderMode
  {
    VOLUME_MODE,
    ENERGY_MODE
  };

  void SetRenderMode(RenderMode mode);
  void SetSampleDistance(const svtkm::Float32&);
  void SetCanvas(svtkm::rendering::Canvas* canvas);
  void SetScalarField(const std::string& fieldName);
  void SetEmissionField(const std::string& fieldName);
  void SetCamera(const svtkm::rendering::Camera& camera);
  void SetScalarRange(const svtkm::Range& range);
  void SetColorMap(svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colormap);
  void SetCompositeBackground(bool on);
  void SetDebugPrints(bool on);
  void SetUnitScalar(svtkm::Float32 unitScalar);

  svtkm::Bounds GetSpatialBounds();
  svtkm::Range GetScalarFieldRange();

  void Trace(const svtkm::rendering::Camera& camera, svtkm::rendering::CanvasRayTracer* canvas);
  void Trace(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays);
  void Trace(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays);

  PartialVector64 PartialTrace(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays);
  PartialVector32 PartialTrace(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays);

protected:
  struct InternalsType;
  struct BoundsFunctor;
  std::shared_ptr<InternalsType> Internals;

private:
  // Do not allow the default constructor
  ConnectivityProxy();
};
}
} //namespace svtkm::rendering
#endif //svtk_m_rendering_SceneRendererVolume_h
