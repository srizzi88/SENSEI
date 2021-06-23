//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/ConnectivityProxy.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/raytracing/ConnectivityTracer.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>


namespace svtkm
{
namespace rendering
{
struct ConnectivityProxy::InternalsType
{
protected:
  using ColorMapType = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
  using TracerType = svtkm::rendering::raytracing::ConnectivityTracer;

  TracerType Tracer;
  svtkm::cont::Field ScalarField;
  svtkm::cont::Field EmissionField;
  svtkm::cont::DynamicCellSet Cells;
  svtkm::cont::CoordinateSystem Coords;
  RenderMode Mode;
  svtkm::Bounds SpatialBounds;
  ColorMapType ColorMap;
  svtkm::cont::DataSet Dataset;
  svtkm::Range ScalarRange;
  bool CompositeBackground;

public:
  InternalsType(svtkm::cont::DataSet& dataSet)
  {
    Dataset = dataSet;
    Cells = dataSet.GetCellSet();
    Coords = dataSet.GetCoordinateSystem();
    Mode = VOLUME_MODE;
    CompositeBackground = true;
    //
    // Just grab a default scalar field
    //

    if (Dataset.GetNumberOfFields() > 0)
    {
      this->SetScalarField(Dataset.GetField(0).GetName());
    }
  }

  ~InternalsType() {}

  SVTKM_CONT
  void SetUnitScalar(svtkm::Float32 unitScalar) { Tracer.SetUnitScalar(unitScalar); }

  void SetSampleDistance(const svtkm::Float32& distance)
  {
    if (Mode != VOLUME_MODE)
    {
      std::cout << "Volume Tracer Error: must set volume mode before setting sample dist\n";
      return;
    }
    Tracer.SetSampleDistance(distance);
  }

  SVTKM_CONT
  void SetRenderMode(RenderMode mode) { Mode = mode; }

  SVTKM_CONT
  RenderMode GetRenderMode() { return Mode; }

  SVTKM_CONT
  void SetScalarField(const std::string& fieldName)
  {
    ScalarField = Dataset.GetField(fieldName);
    const svtkm::cont::ArrayHandle<svtkm::Range> range = this->ScalarField.GetRange();
    ScalarRange = range.GetPortalConstControl().Get(0);
  }

  SVTKM_CONT
  void SetColorMap(svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colormap)
  {
    Tracer.SetColorMap(colormap);
  }

  SVTKM_CONT
  void SetCompositeBackground(bool on) { CompositeBackground = on; }

  SVTKM_CONT
  void SetDebugPrints(bool on) { Tracer.SetDebugOn(on); }

  SVTKM_CONT
  void SetEmissionField(const std::string& fieldName)
  {
    if (Mode != ENERGY_MODE)
    {
      std::cout << "Volume Tracer Error: must set energy mode before setting emission field\n";
      return;
    }
    EmissionField = Dataset.GetField(fieldName);
  }

  SVTKM_CONT
  svtkm::Bounds GetSpatialBounds() const { return SpatialBounds; }

  SVTKM_CONT
  svtkm::Range GetScalarFieldRange()
  {
    const svtkm::cont::ArrayHandle<svtkm::Range> range = this->ScalarField.GetRange();
    ScalarRange = range.GetPortalConstControl().Get(0);
    return ScalarRange;
  }

  SVTKM_CONT
  void SetScalarRange(const svtkm::Range& range) { ScalarRange = range; }

  SVTKM_CONT
  void Trace(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays)
  {

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    Tracer.FullTrace(rays);
  }

  SVTKM_CONT
  void Trace(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays)
  {
    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    Tracer.FullTrace(rays);
  }

  SVTKM_CONT
  PartialVector64 PartialTrace(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays)
  {

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    return Tracer.PartialTrace(rays);
  }

  SVTKM_CONT
  PartialVector32 PartialTrace(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays)
  {
    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      Tracer.SetEnergyData(this->ScalarField,
                           rays.Buffers.at(0).GetNumChannels(),
                           this->Cells,
                           this->Coords,
                           this->EmissionField);
    }

    return Tracer.PartialTrace(rays);
  }

  SVTKM_CONT
  void Trace(const svtkm::rendering::Camera& camera, svtkm::rendering::CanvasRayTracer* canvas)
  {

    if (canvas == nullptr)
    {
      std::cout << "Conn proxy: canvas is NULL\n";
      throw svtkm::cont::ErrorBadValue("Conn Proxy: null canvas");
    }
    svtkm::rendering::raytracing::Camera rayCamera;
    rayCamera.SetParameters(camera, *canvas);
    svtkm::rendering::raytracing::Ray<svtkm::Float32> rays;
    rayCamera.CreateRays(rays, this->Coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *canvas);

    if (Mode == VOLUME_MODE)
    {
      Tracer.SetVolumeData(this->ScalarField, this->ScalarRange, this->Cells, this->Coords);
    }
    else
    {
      throw svtkm::cont::ErrorBadValue("ENERGY MODE Not implemented for this use case\n");
    }

    Tracer.FullTrace(rays);

    canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
    if (CompositeBackground)
    {
      canvas->BlendBackground();
    }
  }
};


SVTKM_CONT
ConnectivityProxy::ConnectivityProxy(svtkm::cont::DataSet& dataSet)
  : Internals(new InternalsType(dataSet))
{
}

SVTKM_CONT
ConnectivityProxy::ConnectivityProxy(const svtkm::cont::DynamicCellSet& cellset,
                                     const svtkm::cont::CoordinateSystem& coords,
                                     const svtkm::cont::Field& scalarField)
{
  svtkm::cont::DataSet dataset;

  dataset.SetCellSet(cellset);
  dataset.AddCoordinateSystem(coords);
  dataset.AddField(scalarField);

  Internals = std::shared_ptr<InternalsType>(new InternalsType(dataset));
}

SVTKM_CONT
ConnectivityProxy::~ConnectivityProxy()
{
}

SVTKM_CONT
ConnectivityProxy::ConnectivityProxy()
{
}

SVTKM_CONT
void ConnectivityProxy::SetSampleDistance(const svtkm::Float32& distance)
{
  Internals->SetSampleDistance(distance);
}

SVTKM_CONT
void ConnectivityProxy::SetRenderMode(RenderMode mode)
{
  Internals->SetRenderMode(mode);
}

SVTKM_CONT
void ConnectivityProxy::SetScalarField(const std::string& fieldName)
{
  Internals->SetScalarField(fieldName);
}

SVTKM_CONT
void ConnectivityProxy::SetColorMap(svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colormap)
{
  Internals->SetColorMap(colormap);
}

SVTKM_CONT
void ConnectivityProxy::SetEmissionField(const std::string& fieldName)
{
  Internals->SetEmissionField(fieldName);
}

SVTKM_CONT
svtkm::Bounds ConnectivityProxy::GetSpatialBounds()
{
  return Internals->GetSpatialBounds();
}

SVTKM_CONT
svtkm::Range ConnectivityProxy::GetScalarFieldRange()
{
  return Internals->GetScalarFieldRange();
}

SVTKM_CONT
void ConnectivityProxy::SetCompositeBackground(bool on)
{
  return Internals->SetCompositeBackground(on);
}

SVTKM_CONT
void ConnectivityProxy::SetScalarRange(const svtkm::Range& range)
{
  Internals->SetScalarRange(range);
}

SVTKM_CONT
void ConnectivityProxy::Trace(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_64");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);
  logger->CloseLogEntry(-1.0);
}

SVTKM_CONT
PartialVector32 ConnectivityProxy::PartialTrace(
  svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  PartialVector32 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
}

SVTKM_CONT
void ConnectivityProxy::Trace(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  Internals->Trace(rays);

  logger->CloseLogEntry(-1.0);
}

SVTKM_CONT
PartialVector64 ConnectivityProxy::PartialTrace(
  svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_64");
  if (Internals->GetRenderMode() == VOLUME_MODE)
  {
    logger->AddLogData("volume_mode", "true");
  }
  else
  {
    logger->AddLogData("volume_mode", "false");
  }

  PartialVector64 res = Internals->PartialTrace(rays);

  logger->CloseLogEntry(-1.0);
  return res;
}

SVTKM_CONT
void ConnectivityProxy::Trace(const svtkm::rendering::Camera& camera,
                              svtkm::rendering::CanvasRayTracer* canvas)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("connectivity_trace_32");
  logger->AddLogData("volume_mode", "true");

  Internals->Trace(camera, canvas);

  logger->CloseLogEntry(-1.0);
}

SVTKM_CONT
void ConnectivityProxy::SetDebugPrints(bool on)
{
  Internals->SetDebugPrints(on);
}

SVTKM_CONT
void ConnectivityProxy::SetUnitScalar(svtkm::Float32 unitScalar)
{
  Internals->SetUnitScalar(unitScalar);
}
}
} // namespace svtkm::rendering
