//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <complex>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>
#include <svtkm/worklet/Tube.h>

#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperRayTracer.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>

svtkm::Vec3f ArchimedeanSpiralToCartesian(svtkm::Vec3f const& p)
{
  // p[0] = r, p[1] = theta, p[2] = z:
  svtkm::Vec3f xyz;
  auto c = std::polar(p[0], p[1]);
  xyz[0] = c.real();
  xyz[1] = c.imag();
  xyz[2] = p[2];
  return xyz;
}

void TubeThatSpiral(svtkm::FloatDefault radius, svtkm::Id numLineSegments, svtkm::Id numSides)
{
  svtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<svtkm::Id> ids;

  // The Archimedian spiral is defined by the equation r = a + b*theta.
  // To extend to a 3D curve, use z = t, theta = t, r = a + b t.
  svtkm::FloatDefault a = svtkm::FloatDefault(0.2);
  svtkm::FloatDefault b = svtkm::FloatDefault(0.8);
  for (svtkm::Id i = 0; i < numLineSegments; ++i)
  {
    svtkm::FloatDefault t = 4 * svtkm::FloatDefault(3.1415926) * (i + 1) /
      numLineSegments; // roughly two spins around. Doesn't need to be perfect.
    svtkm::FloatDefault r = a + b * t;
    svtkm::FloatDefault theta = t;
    svtkm::Vec3f cylindricalCoordinate{ r, theta, t };
    svtkm::Vec3f spiralSample = ArchimedeanSpiralToCartesian(cylindricalCoordinate);
    svtkm::Id pid = dsb.AddPoint(spiralSample);
    ids.push_back(pid);
  }
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  svtkm::cont::DataSet ds = dsb.Create();

  svtkm::worklet::Tube tubeWorklet(
    /*capEnds = */ true,
    /* how smooth the cylinder is; infinitely smooth as n->infty */ numSides,
    radius);

  // You added lines, but you need to extend it to a tube.
  // This generates a new pointset, and new cell set.
  svtkm::cont::ArrayHandle<svtkm::Vec3f> tubePoints;
  svtkm::cont::CellSetSingleType<> tubeCells;
  tubeWorklet.Run(ds.GetCoordinateSystem().GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Vec3f>>(),
                  ds.GetCellSet(),
                  tubePoints,
                  tubeCells);

  svtkm::cont::DataSet tubeDataset;
  tubeDataset.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", tubePoints));
  tubeDataset.SetCellSet(tubeCells);

  svtkm::Bounds coordsBounds = tubeDataset.GetCoordinateSystem().GetBounds();

  svtkm::Vec3f_64 totalExtent(
    coordsBounds.X.Length(), coordsBounds.Y.Length(), coordsBounds.Z.Length());
  svtkm::Float64 mag = svtkm::Magnitude(totalExtent);
  svtkm::Normalize(totalExtent);

  // setup a camera and point it to towards the center of the input data
  svtkm::rendering::Camera camera;
  camera.ResetToBounds(coordsBounds);

  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(svtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 100.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(totalExtent * (mag * 2.f));
  svtkm::cont::ColorTable colorTable("inferno");

  svtkm::rendering::Scene scene;
  svtkm::rendering::MapperRayTracer mapper;
  svtkm::rendering::CanvasRayTracer canvas(2048, 2048);
  svtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);


  svtkm::cont::DataSetFieldAdd dsfa;
  std::vector<svtkm::FloatDefault> v(static_cast<std::size_t>(tubePoints.GetNumberOfValues()));
  // The first value is a cap:
  v[0] = 0;
  for (svtkm::Id i = 1; i < svtkm::Id(v.size()); i += numSides)
  {
    svtkm::FloatDefault t = 4 * svtkm::FloatDefault(3.1415926) * (i + 1) / numSides;
    svtkm::FloatDefault r = a + b * t;
    for (svtkm::Id j = i; j < i + numSides && j < svtkm::Id(v.size()); ++j)
    {
      v[static_cast<std::size_t>(j)] = r;
    }
  }
  // Point at the end cap should be the same color as the surroundings:
  v[v.size() - 1] = v[v.size() - 2];

  dsfa.AddPointField(tubeDataset, "Spiral Radius", v);
  scene.AddActor(svtkm::rendering::Actor(tubeDataset.GetCellSet(),
                                        tubeDataset.GetCoordinateSystem(),
                                        tubeDataset.GetField("Spiral Radius"),
                                        colorTable));
  svtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Initialize();
  view.Paint();
  std::string output_filename = "tube_output_" + std::to_string(numSides) + "_sides.pnm";
  view.SaveAs(output_filename);
}



int main()
{
  // Radius of the tube:
  svtkm::FloatDefault radius = 0.5f;
  // How many segments is the tube decomposed into?
  svtkm::Id numLineSegments = 100;
  // As numSides->infty, the tubes becomes perfectly cylindrical:
  svtkm::Id numSides = 50;
  TubeThatSpiral(radius, numLineSegments, numSides);
  // Setting numSides = 4 makes a square around the polyline:
  numSides = 4;
  TubeThatSpiral(radius, numLineSegments, numSides);
  return 0;
}
