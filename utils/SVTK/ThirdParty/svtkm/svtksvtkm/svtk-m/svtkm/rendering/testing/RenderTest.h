//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_testing_RenderTest_h
#define svtk_m_rendering_testing_RenderTest_h

#include <svtkm/Bounds.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/TextAnnotationScreen.h>
#include <svtkm/rendering/View1D.h>
#include <svtkm/rendering/View2D.h>
#include <svtkm/rendering/View3D.h>

#include <memory>

namespace svtkm
{
namespace rendering
{
namespace testing
{

template <typename ViewType>
inline void SetCamera(svtkm::rendering::Camera& camera,
                      const svtkm::Bounds& coordBounds,
                      const svtkm::cont::Field& field);
template <typename ViewType>
inline void SetCamera(svtkm::rendering::Camera& camera,
                      const svtkm::Bounds& coordBounds,
                      const svtkm::cont::Field& field);

template <>
inline void SetCamera<svtkm::rendering::View3D>(svtkm::rendering::Camera& camera,
                                               const svtkm::Bounds& coordBounds,
                                               const svtkm::cont::Field&)
{
  svtkm::Bounds b = coordBounds;
  b.Z.Min = 0;
  b.Z.Max = 4;
  camera = svtkm::rendering::Camera();
  camera.ResetToBounds(b);
  camera.Azimuth(static_cast<svtkm::Float32>(45.0));
  camera.Elevation(static_cast<svtkm::Float32>(45.0));
}

template <>
inline void SetCamera<svtkm::rendering::View2D>(svtkm::rendering::Camera& camera,
                                               const svtkm::Bounds& coordBounds,
                                               const svtkm::cont::Field&)
{
  camera = svtkm::rendering::Camera(svtkm::rendering::Camera::MODE_2D);
  camera.ResetToBounds(coordBounds);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}

template <>
inline void SetCamera<svtkm::rendering::View1D>(svtkm::rendering::Camera& camera,
                                               const svtkm::Bounds& coordBounds,
                                               const svtkm::cont::Field& field)
{
  svtkm::Bounds bounds;
  bounds.X = coordBounds.X;
  field.GetRange(&bounds.Y);

  camera = svtkm::rendering::Camera(svtkm::rendering::Camera::MODE_2D);
  camera.ResetToBounds(bounds);
  camera.SetClippingRange(1.f, 100.f);
  camera.SetViewport(-0.7f, +0.7f, -0.7f, +0.7f);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(ViewType& view, const std::string& outputFile)
{
  view.Initialize();
  view.Paint();
  view.SaveAs(outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const svtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const svtkm::cont::ColorTable& colorTable,
            const std::string& outputFile)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  svtkm::rendering::Scene scene;

  scene.AddActor(svtkm::rendering::Actor(
    ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fieldNm), colorTable));
  svtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fieldNm));
  svtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  svtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  ViewType view(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<svtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new svtkm::rendering::TextAnnotationScreen(
      "Test Plot", svtkm::rendering::Color(1, 1, 1, 1), .075f, svtkm::Vec2f_32(-.11f, .92f), 0.f));
  view.AddAnnotation(std::move(titleAnnotation));
  Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

// A render test that allows for testing different mapper params
template <typename MapperType, typename CanvasType, typename ViewType>
void Render(MapperType& mapper,
            const svtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const svtkm::cont::ColorTable& colorTable,
            const std::string& outputFile)
{
  CanvasType canvas(512, 512);
  svtkm::rendering::Scene scene;

  scene.AddActor(svtkm::rendering::Actor(
    ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fieldNm), colorTable));
  svtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fieldNm));
  svtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  svtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  ViewType view(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<svtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new svtkm::rendering::TextAnnotationScreen(
      "Test Plot", svtkm::rendering::Color(1, 1, 1, 1), .075f, svtkm::Vec2f_32(-.11f, .92f), 0.f));
  view.AddAnnotation(std::move(titleAnnotation));
  Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const svtkm::cont::DataSet& ds,
            const std::vector<std::string>& fields,
            const std::vector<svtkm::rendering::Color>& colors,
            const std::string& outputFile)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  canvas.SetBackgroundColor(svtkm::rendering::Color::white);
  svtkm::rendering::Scene scene;

  size_t numFields = fields.size();
  for (size_t i = 0; i < numFields; ++i)
  {
    scene.AddActor(svtkm::rendering::Actor(
      ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fields[i]), colors[i]));
  }
  svtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fields[0]));

  svtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  svtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  ViewType view(scene, mapper, canvas, camera, background, foreground);

  // Print the title
  std::unique_ptr<svtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new svtkm::rendering::TextAnnotationScreen(
      "Test Plot", svtkm::rendering::Color(1, 1, 1, 1), .075f, svtkm::Vec2f_32(-.11f, .92f), 0.f));
  view.AddAnnotation(std::move(titleAnnotation));
  Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

template <typename MapperType, typename CanvasType, typename ViewType>
void Render(const svtkm::cont::DataSet& ds,
            const std::string& fieldNm,
            const svtkm::rendering::Color& color,
            const std::string& outputFile,
            const bool logY = false)
{
  MapperType mapper;
  CanvasType canvas(512, 512);
  svtkm::rendering::Scene scene;

  //DRP Actor? no field? no colortable (or a constant colortable) ??
  scene.AddActor(
    svtkm::rendering::Actor(ds.GetCellSet(), ds.GetCoordinateSystem(), ds.GetField(fieldNm), color));
  svtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, ds.GetCoordinateSystem().GetBounds(), ds.GetField(fieldNm));

  svtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  svtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);

  ViewType view(scene, mapper, canvas, camera, background, foreground);
  // Print the title
  std::unique_ptr<svtkm::rendering::TextAnnotationScreen> titleAnnotation(
    new svtkm::rendering::TextAnnotationScreen(
      "1D Test Plot", foreground, .1f, svtkm::Vec2f_32(-.27f, .87f), 0.f));
  view.AddAnnotation(std::move(titleAnnotation));
  view.SetLogY(logY);
  Render<MapperType, CanvasType, ViewType>(view, outputFile);
}

template <typename MapperType1, typename MapperType2, typename CanvasType, typename ViewType>
void MultiMapperRender(const svtkm::cont::DataSet& ds1,
                       const svtkm::cont::DataSet& ds2,
                       const std::string& fieldNm,
                       const svtkm::cont::ColorTable& colorTable1,
                       const svtkm::cont::ColorTable& colorTable2,
                       const std::string& outputFile)
{
  MapperType1 mapper1;
  MapperType2 mapper2;

  CanvasType canvas(512, 512);
  canvas.SetBackgroundColor(svtkm::rendering::Color(0.8f, 0.8f, 0.8f, 1.0f));
  canvas.Clear();

  svtkm::Bounds totalBounds =
    ds1.GetCoordinateSystem().GetBounds() + ds2.GetCoordinateSystem().GetBounds();
  svtkm::rendering::Camera camera;
  SetCamera<ViewType>(camera, totalBounds, ds1.GetField(fieldNm));

  mapper1.SetCanvas(&canvas);
  mapper1.SetActiveColorTable(colorTable1);
  mapper1.SetCompositeBackground(false);

  mapper2.SetCanvas(&canvas);
  mapper2.SetActiveColorTable(colorTable2);

  const svtkm::cont::Field field1 = ds1.GetField(fieldNm);
  svtkm::Range range1;
  field1.GetRange(&range1);

  const svtkm::cont::Field field2 = ds2.GetField(fieldNm);
  svtkm::Range range2;
  field2.GetRange(&range2);

  mapper1.RenderCells(
    ds1.GetCellSet(), ds1.GetCoordinateSystem(), field1, colorTable1, camera, range1);

  mapper2.RenderCells(
    ds2.GetCellSet(), ds2.GetCoordinateSystem(), field2, colorTable2, camera, range2);

  canvas.SaveAs(outputFile);
}
}
}
} // namespace svtkm::rendering::testing

#endif //svtk_m_rendering_testing_RenderTest_h
