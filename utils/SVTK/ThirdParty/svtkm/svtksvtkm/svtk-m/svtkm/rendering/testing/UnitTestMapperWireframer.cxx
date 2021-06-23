//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperWireframer.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

svtkm::cont::DataSet Make3DUniformDataSet(svtkm::Id size = 64)
{
  svtkm::Float32 center = static_cast<svtkm::Float32>(-size) / 2.0f;
  svtkm::cont::DataSetBuilderUniform builder;
  svtkm::cont::DataSet dataSet = builder.Create(svtkm::Id3(size, size, size),
                                               svtkm::Vec3f_32(center, center, center),
                                               svtkm::Vec3f_32(1.0f, 1.0f, 1.0f));
  const char* fieldName = "pointvar";
  svtkm::Id numValues = dataSet.GetNumberOfPoints();
  svtkm::cont::ArrayHandleCounting<svtkm::Float32> fieldValues(
    0.0f, 10.0f / static_cast<svtkm::Float32>(numValues), numValues);
  svtkm::cont::ArrayHandle<svtkm::Float32> scalarField;
  svtkm::cont::ArrayCopy(fieldValues, scalarField);
  svtkm::cont::DataSetFieldAdd().AddPointField(dataSet, fieldName, scalarField);
  return dataSet;
}

svtkm::cont::DataSet Make2DExplicitDataSet()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  const int nVerts = 5;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);
  CoordType coordinates[nVerts] = { CoordType(0.f, 0.f, 0.f),
                                    CoordType(1.f, .5f, 0.f),
                                    CoordType(2.f, 1.f, 0.f),
                                    CoordType(3.f, 1.7f, 0.f),
                                    CoordType(4.f, 3.f, 0.f) };

  std::vector<svtkm::Float32> cellVar;
  cellVar.push_back(10);
  cellVar.push_back(12);
  cellVar.push_back(13);
  cellVar.push_back(14);
  std::vector<svtkm::Float32> pointVar;
  pointVar.push_back(10);
  pointVar.push_back(12);
  pointVar.push_back(13);
  pointVar.push_back(14);
  pointVar.push_back(15);
  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));
  svtkm::cont::CellSetSingleType<> cellSet;

  svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
  connectivity.Allocate(8);
  auto connPortal = connectivity.GetPortalControl();
  connPortal.Set(0, 0);
  connPortal.Set(1, 1);

  connPortal.Set(2, 1);
  connPortal.Set(3, 2);

  connPortal.Set(4, 2);
  connPortal.Set(5, 3);

  connPortal.Set(6, 3);
  connPortal.Set(7, 4);

  cellSet.Fill(nVerts, svtkm::CELL_SHAPE_LINE, 2, connectivity);
  dataSet.SetCellSet(cellSet);
  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointVar", pointVar);
  dsf.AddCellField(dataSet, "cellVar", cellVar);

  return dataSet;
}

void RenderTests()
{
  using M = svtkm::rendering::MapperWireframer;
  using C = svtkm::rendering::CanvasRayTracer;
  using V3 = svtkm::rendering::View3D;
  using V2 = svtkm::rendering::View2D;
  using V1 = svtkm::rendering::View1D;

  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("samsel fire");

  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "wf_reg3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "wf_rect3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "wf_expl3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    Make3DUniformDataSet(), "pointvar", colorTable, "wf_uniform3D.pnm");
  svtkm::rendering::testing::Render<M, C, V2>(
    Make2DExplicitDataSet(), "cellVar", colorTable, "wf_lines2D.pnm");
  //
  // Test the 1D cell set line plot with multiple lines
  //
  std::vector<std::string> fields;
  fields.push_back("pointvar");
  fields.push_back("pointvar2");
  std::vector<svtkm::rendering::Color> colors;
  colors.push_back(svtkm::rendering::Color(1.f, 0.f, 0.f));
  colors.push_back(svtkm::rendering::Color(0.f, 1.f, 0.f));
  svtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet0(), fields, colors, "wf_lines1D.pnm");
  //test log y
  svtkm::rendering::Color red = svtkm::rendering::Color::red;
  svtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet1(), "pointvar", red, "wf_linesLogY1D.pnm", true);
}

} //namespace

int UnitTestMapperWireframer(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
