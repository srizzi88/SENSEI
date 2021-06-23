//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_testing_MakeTestDataSet_h
#define svtk_m_cont_testing_MakeTestDataSet_h

#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>

#include <svtkm/cont/testing/Testing.h>

#include <numeric>

namespace svtkm
{
namespace cont
{
namespace testing
{

class MakeTestDataSet
{
public:
  // 1D uniform datasets.
  svtkm::cont::DataSet Make1DUniformDataSet0();
  svtkm::cont::DataSet Make1DUniformDataSet1();
  svtkm::cont::DataSet Make1DUniformDataSet2();

  // 1D explicit datasets.
  svtkm::cont::DataSet Make1DExplicitDataSet0();

  // 2D uniform datasets.
  svtkm::cont::DataSet Make2DUniformDataSet0();
  svtkm::cont::DataSet Make2DUniformDataSet1();
  svtkm::cont::DataSet Make2DUniformDataSet2();

  // 3D uniform datasets.
  svtkm::cont::DataSet Make3DUniformDataSet0();
  svtkm::cont::DataSet Make3DUniformDataSet1();
  svtkm::cont::DataSet Make3DUniformDataSet2();
  svtkm::cont::DataSet Make3DUniformDataSet3(const svtkm::Id3 dims = svtkm::Id3(10));
  svtkm::cont::DataSet Make3DRegularDataSet0();
  svtkm::cont::DataSet Make3DRegularDataSet1();

  //2D rectilinear
  svtkm::cont::DataSet Make2DRectilinearDataSet0();

  //3D rectilinear
  svtkm::cont::DataSet Make3DRectilinearDataSet0();

  // 2D explicit datasets.
  svtkm::cont::DataSet Make2DExplicitDataSet0();

  // 3D explicit datasets.
  svtkm::cont::DataSet Make3DExplicitDataSet0();
  svtkm::cont::DataSet Make3DExplicitDataSet1();
  svtkm::cont::DataSet Make3DExplicitDataSet2();
  svtkm::cont::DataSet Make3DExplicitDataSet3();
  svtkm::cont::DataSet Make3DExplicitDataSet4();
  svtkm::cont::DataSet Make3DExplicitDataSet5();
  svtkm::cont::DataSet Make3DExplicitDataSet6();
  svtkm::cont::DataSet Make3DExplicitDataSet7();
  svtkm::cont::DataSet Make3DExplicitDataSet8();
  svtkm::cont::DataSet Make3DExplicitDataSetZoo();
  svtkm::cont::DataSet Make3DExplicitDataSetPolygonal();
  svtkm::cont::DataSet Make3DExplicitDataSetCowNose();
};

//Make a simple 1D dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet0()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  const svtkm::Id nVerts = 6;
  svtkm::cont::DataSet dataSet = dsb.Create(nVerts);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Float32 var[nVerts] = { -1.0f, .5f, -.2f, 1.7f, -.1f, .8f };
  constexpr svtkm::Float32 var2[nVerts] = { -1.1f, .7f, -.2f, 0.2f, -.1f, .4f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);
  dsf.AddPointField(dataSet, "pointvar2", var2, nVerts);

  return dataSet;
}

//Make another simple 1D dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet1()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  const svtkm::Id nVerts = 6;
  svtkm::cont::DataSet dataSet = dsb.Create(nVerts);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Float32 var[nVerts] = { 1.0e3f, 5.e5f, 2.e8f, 1.e10f, 2e12f, 3e15f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  return dataSet;
}


//Make a simple 1D, 16 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet2()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id dims = 256;
  svtkm::cont::DataSet dataSet = dsb.Create(dims);

  svtkm::cont::DataSetFieldAdd dsf;
  svtkm::Float64 pointvar[dims];
  constexpr svtkm::Float64 dx = svtkm::Float64(4.0 * svtkm::Pi()) / svtkm::Float64(dims - 1);

  svtkm::Id idx = 0;
  for (svtkm::Id x = 0; x < dims; ++x)
  {
    svtkm::Float64 cx = svtkm::Float64(x) * dx - 2.0 * svtkm::Pi();
    svtkm::Float64 cv = svtkm::Sin(cx);

    pointvar[idx] = cv;
    idx++;
  }

  dsf.AddPointField(dataSet, "pointvar", pointvar, dims);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make1DExplicitDataSet0()
{
  const int nVerts = 5;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);
  coords[0] = CoordType(0.0f, 0.f, 0.f);
  coords[1] = CoordType(1.0f, 0.f, 0.f);
  coords[2] = CoordType(1.1f, 0.f, 0.f);
  coords[3] = CoordType(1.2f, 0.f, 0.f);
  coords[4] = CoordType(4.0f, 0.f, 0.f);

  // Each line connects two consecutive vertices
  std::vector<svtkm::Id> conn;
  for (int i = 0; i < nVerts - 1; i++)
  {
    conn.push_back(i);
    conn.push_back(i + 1);
  }

  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;

  dataSet = dsb.Create(coords, svtkm::CellShapeTagLine(), 2, conn, "coordinates");

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Float32 var[nVerts] = { -1.0f, .5f, -.2f, 1.7f, .8f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  return dataSet;
}

//Make a simple 2D, 2 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet0()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id2 dimensions(3, 2);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Id nVerts = 6;
  constexpr svtkm::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };

  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  constexpr svtkm::Float32 cellvar[2] = { 100.1f, 200.1f };
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2);

  return dataSet;
}

//Make a simple 2D, 16 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet1()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id2 dimensions(5, 5);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Id nVerts = 25;
  constexpr svtkm::Id nCells = 16;
  constexpr svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 1.0f,  94.0f, 71.0f,
                                               47.0f,  33.0f, 6.0f,  52.0f, 44.0f, 50.0f, 45.0f,
                                               48.0f,  8.0f,  12.0f, 46.0f, 91.0f, 43.0f, 0.0f,
                                               5.0f,   51.0f, 76.0f, 83.0f };
  constexpr svtkm::Float32 cellvar[nCells] = {
    0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
    8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f
  };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

//Make a simple 2D, 16 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet2()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id2 dims(16, 16);
  svtkm::cont::DataSet dataSet = dsb.Create(dims);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Id nVerts = 256;
  svtkm::Float64 pointvar[nVerts];
  constexpr svtkm::Float64 dx = svtkm::Float64(4.0 * svtkm::Pi()) / svtkm::Float64(dims[0] - 1);
  constexpr svtkm::Float64 dy = svtkm::Float64(2.0 * svtkm::Pi()) / svtkm::Float64(dims[1] - 1);

  svtkm::Id idx = 0;
  for (svtkm::Id y = 0; y < dims[1]; ++y)
  {
    svtkm::Float64 cy = svtkm::Float64(y) * dy - svtkm::Pi();
    for (svtkm::Id x = 0; x < dims[0]; ++x)
    {
      svtkm::Float64 cx = svtkm::Float64(x) * dx - 2.0 * svtkm::Pi();
      svtkm::Float64 cv = svtkm::Sin(cx) + svtkm::Sin(cy) +
        2.0 * svtkm::Cos(svtkm::Sqrt((cx * cx) / 2.0 + cy * cy) / 0.75) +
        4.0 * svtkm::Cos(cx * cy / 4.0);

      pointvar[idx] = cv;
      idx++;
    }
  } // y

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);

  return dataSet;
}
//Make a simple 3D, 4 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet0()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id3 dimensions(3, 2, 3);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr int nVerts = 18;
  constexpr svtkm::Float32 vars[nVerts] = { 10.1f,  20.1f,  30.1f,  40.1f,  50.2f,  60.2f,
                                           70.2f,  80.2f,  90.3f,  100.3f, 110.3f, 120.3f,
                                           130.4f, 140.4f, 150.4f, 160.4f, 170.5f, 180.5f };

  //Set point and cell scalar
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);

  constexpr svtkm::Float32 cellvar[4] = { 100.1f, 100.2f, 100.3f, 100.4f };
  dsf.AddCellField(dataSet, "cellvar", cellvar, 4);

  return dataSet;
}

//Make a simple 3D, 64 cell uniform dataset.
inline svtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet1()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  constexpr svtkm::Id3 dimensions(5, 5, 5);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Id nVerts = 125;
  constexpr svtkm::Id nCells = 64;
  constexpr svtkm::Float32 pointvar[nVerts] = {
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f,
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  99.0f, 90.0f, 85.0f, 0.0f, 0.0f, 95.0f, 80.0f,
    95.0f, 0.0f, 0.0f, 85.0f, 90.0f, 99.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  75.0f, 50.0f, 65.0f, 0.0f, 0.0f, 55.0f, 15.0f,
    45.0f, 0.0f, 0.0f, 60.0f, 40.0f, 70.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  97.0f, 87.0f, 82.0f, 0.0f, 0.0f, 92.0f, 77.0f,
    92.0f, 0.0f, 0.0f, 82.0f, 87.0f, 97.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f,
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f
  };
  constexpr svtkm::Float32 cellvar[nCells] = {
    0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
    8.0f,  9.0f,  10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,

    16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f,
    24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f,

    32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f,
    40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f,

    48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f,
    56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f
  };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet2()
{
  constexpr svtkm::Id base_size = 256;
  svtkm::cont::DataSetBuilderUniform dsb;
  svtkm::Id3 dimensions(base_size, base_size, base_size);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  constexpr svtkm::Id nVerts = base_size * base_size * base_size;
  svtkm::Float32* pointvar = new svtkm::Float32[nVerts];

  for (svtkm::Id z = 0; z < base_size; ++z)
    for (svtkm::Id y = 0; y < base_size; ++y)
      for (svtkm::Id x = 0; x < base_size; ++x)
      {
        std::size_t index = static_cast<std::size_t>(z * base_size * base_size + y * base_size + x);
        pointvar[index] = svtkm::Sqrt(svtkm::Float32(x * x + y * y + z * z));
      }

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);

  delete[] pointvar;

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet3(const svtkm::Id3 dims)
{
  svtkm::cont::DataSetBuilderUniform dsb;
  svtkm::cont::DataSet dataSet = dsb.Create(dims);

  // add point scalar field
  svtkm::Id numPoints = dims[0] * dims[1] * dims[2];
  std::vector<svtkm::Float64> pointvar(static_cast<size_t>(numPoints));

  svtkm::Float64 dx = svtkm::Float64(4.0 * svtkm::Pi()) / svtkm::Float64(dims[0] - 1);
  svtkm::Float64 dy = svtkm::Float64(2.0 * svtkm::Pi()) / svtkm::Float64(dims[1] - 1);
  svtkm::Float64 dz = svtkm::Float64(3.0 * svtkm::Pi()) / svtkm::Float64(dims[2] - 1);

  svtkm::Id idx = 0;
  for (svtkm::Id z = 0; z < dims[2]; ++z)
  {
    svtkm::Float64 cz = svtkm::Float64(z) * dz - 1.5 * svtkm::Pi();
    for (svtkm::Id y = 0; y < dims[1]; ++y)
    {
      svtkm::Float64 cy = svtkm::Float64(y) * dy - svtkm::Pi();
      for (svtkm::Id x = 0; x < dims[0]; ++x)
      {
        svtkm::Float64 cx = svtkm::Float64(x) * dx - 2.0 * svtkm::Pi();
        svtkm::Float64 cv = svtkm::Sin(cx) + svtkm::Sin(cy) +
          2.0 * svtkm::Cos(svtkm::Sqrt((cx * cx) / 2.0 + cy * cy) / 0.75) +
          4.0 * svtkm::Cos(cx * cy / 4.0);

        if (dims[2] > 1)
        {
          cv += svtkm::Sin(cz) + 1.5 * svtkm::Cos(svtkm::Sqrt(cx * cx + cy * cy + cz * cz) / 0.75);
        }
        pointvar[static_cast<size_t>(idx)] = cv;
        idx++;
      }
    } // y
  }   // z

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", pointvar);

  svtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1);
  dsf.AddCellField(
    dataSet,
    "cellvar",
    svtkm::cont::make_ArrayHandleCounting(svtkm::Float64(0), svtkm::Float64(1), numCells));

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make2DRectilinearDataSet0()
{
  svtkm::cont::DataSetBuilderRectilinear dsb;
  std::vector<svtkm::Float32> X(3), Y(2);

  X[0] = 0.0f;
  X[1] = 1.0f;
  X[2] = 2.0f;
  Y[0] = 0.0f;
  Y[1] = 1.0f;

  svtkm::cont::DataSet dataSet = dsb.Create(X, Y);

  svtkm::cont::DataSetFieldAdd dsf;
  const svtkm::Id nVerts = 6;
  svtkm::Float32 var[nVerts];
  for (int i = 0; i < nVerts; i++)
    var[i] = (svtkm::Float32)i;
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  const svtkm::Id nCells = 2;
  svtkm::Float32 cellvar[nCells];
  for (int i = 0; i < nCells; i++)
    cellvar[i] = (svtkm::Float32)i;
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet0()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 18;
  svtkm::cont::ArrayHandleUniformPointCoordinates coordinates(svtkm::Id3(3, 2, 3));
  svtkm::Float32 vars[nVerts] = { 10.1f,  20.1f,  30.1f,  40.1f,  50.2f,  60.2f,
                                 70.2f,  80.2f,  90.3f,  100.3f, 110.3f, 120.3f,
                                 130.4f, 140.4f, 150.4f, 160.4f, 170.5f, 180.5f };

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[4] = { 100.1f, 100.2f, 100.3f, 100.4f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 4, svtkm::CopyFlag::On));

  static constexpr svtkm::IdComponent dim = 3;
  svtkm::cont::CellSetStructured<dim> cellSet;
  cellSet.SetPointDimensions(svtkm::make_Vec(3, 2, 3));
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet1()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 8;
  svtkm::cont::ArrayHandleUniformPointCoordinates coordinates(svtkm::Id3(2, 2, 2));
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.2f, 60.2f, 70.2f, 80.2f };

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[1] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 1, svtkm::CopyFlag::On));

  static constexpr svtkm::IdComponent dim = 3;
  svtkm::cont::CellSetStructured<dim> cellSet;
  cellSet.SetPointDimensions(svtkm::make_Vec(2, 2, 2));
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DRectilinearDataSet0()
{
  svtkm::cont::DataSetBuilderRectilinear dsb;
  std::vector<svtkm::Float32> X(3), Y(2), Z(3);

  X[0] = 0.0f;
  X[1] = 1.0f;
  X[2] = 2.0f;
  Y[0] = 0.0f;
  Y[1] = 1.0f;
  Z[0] = 0.0f;
  Z[1] = 1.0f;
  Z[2] = 2.0f;

  svtkm::cont::DataSet dataSet = dsb.Create(X, Y, Z);

  svtkm::cont::DataSetFieldAdd dsf;
  const svtkm::Id nVerts = 18;
  svtkm::Float32 var[nVerts];
  for (int i = 0; i < nVerts; i++)
    var[i] = (svtkm::Float32)i;
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  const svtkm::Id nCells = 4;
  svtkm::Float32 cellvar[nCells];
  for (int i = 0; i < nCells; i++)
    cellvar[i] = (svtkm::Float32)i;
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

// Make a 2D explicit dataset
inline svtkm::cont::DataSet MakeTestDataSet::Make2DExplicitDataSet0()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 16;
  const int nCells = 7;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(2, 0, 0);
  coords[3] = CoordType(3, 0, 0);
  coords[4] = CoordType(0, 1, 0);
  coords[5] = CoordType(1, 1, 0);
  coords[6] = CoordType(2, 1, 0);
  coords[7] = CoordType(3, 1, 0);
  coords[8] = CoordType(0, 2, 0);
  coords[9] = CoordType(1, 2, 0);
  coords[10] = CoordType(2, 2, 0);
  coords[11] = CoordType(3, 2, 0);
  coords[12] = CoordType(0, 3, 0);
  coords[13] = CoordType(3, 3, 0);
  coords[14] = CoordType(1, 4, 0);
  coords[15] = CoordType(2, 4, 0);

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(10);
  conn.push_back(9);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(11);
  conn.push_back(10);

  shapes.push_back(svtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(6);
  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(13);
  conn.push_back(15);
  conn.push_back(14);
  conn.push_back(12);
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 33.0f,
                                     52.0f,  44.0f, 50.0f, 45.0f, 8.0f,  12.0f, 46.0f, 91.0f };
  svtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet0()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);
  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(1, 1, 0);
  coords[3] = CoordType(2, 1, 0);
  coords[4] = CoordType(2, 2, 0);

  //Connectivity
  std::vector<svtkm::UInt8> shapes;
  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(svtkm::CELL_SHAPE_QUAD);

  std::vector<svtkm::IdComponent> numindices;
  numindices.push_back(3);
  numindices.push_back(4);

  std::vector<svtkm::Id> conn;
  // First Cell: Triangle
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell: Quad
  conn.push_back(2);
  conn.push_back(1);
  conn.push_back(3);
  conn.push_back(4);

  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };
  svtkm::Float32 cellvar[2] = { 100.1f, 100.2f };

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet1()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(1, 1, 0);
  coords[3] = CoordType(2, 1, 0);
  coords[4] = CoordType(2, 2, 0);
  CoordType coordinates[nVerts] = { CoordType(0, 0, 0),
                                    CoordType(1, 0, 0),
                                    CoordType(1, 1, 0),
                                    CoordType(2, 1, 0),
                                    CoordType(2, 2, 0) };
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));
  svtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(svtkm::CELL_SHAPE_TRIANGLE, 3, make_Vec<svtkm::Id>(0, 1, 2));
  cellSet.AddCell(svtkm::CELL_SHAPE_QUAD, 4, make_Vec<svtkm::Id>(2, 1, 3, 4));
  cellSet.CompleteAddingCells(nVerts);
  dataSet.SetCellSet(cellSet);

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[2] = { 100.1f, 100.2f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 2, svtkm::CopyFlag::On));

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet2()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 8;
  using CoordType = svtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f, 70.2f, 80.3f };

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[2] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 1, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  svtkm::Vec<svtkm::Id, 8> ids;
  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 2;
  ids[3] = 3;
  ids[4] = 4;
  ids[5] = 5;
  ids[6] = 6;
  ids[7] = 7;

  cellSet.PrepareToAddCells(1, 8);
  cellSet.AddCell(svtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet4()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 12;
  using CoordType = svtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), //0
    CoordType(1, 0, 0), //1
    CoordType(1, 0, 1), //2
    CoordType(0, 0, 1), //3
    CoordType(0, 1, 0), //4
    CoordType(1, 1, 0), //5
    CoordType(1, 1, 1), //6
    CoordType(0, 1, 1), //7
    CoordType(2, 0, 0), //8
    CoordType(2, 0, 1), //9
    CoordType(2, 1, 1), //10
    CoordType(2, 1, 0)  //11
  };
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f,
                                 70.2f, 80.3f, 90.f,  10.f,  11.f,  12.f };

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[2] = { 100.1f, 110.f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 2, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  svtkm::Vec<svtkm::Id, 8> ids;
  ids[0] = 0;
  ids[1] = 4;
  ids[2] = 5;
  ids[3] = 1;
  ids[4] = 3;
  ids[5] = 7;
  ids[6] = 6;
  ids[7] = 2;

  cellSet.PrepareToAddCells(2, 16);
  cellSet.AddCell(svtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  ids[0] = 1;
  ids[1] = 5;
  ids[2] = 11;
  ids[3] = 8;
  ids[4] = 2;
  ids[5] = 6;
  ids[6] = 10;
  ids[7] = 9;
  cellSet.AddCell(svtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet3()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 4;
  using CoordType = svtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), CoordType(1, 0, 0), CoordType(1, 0, 1), CoordType(0, 1, 0)
  };
  svtkm::Float32 vars[nVerts] = { 10.1f, 10.1f, 10.2f, 30.2f };

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  svtkm::Float32 cellvar[2] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, 1, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  svtkm::Id4 ids;
  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 2;
  ids[3] = 3;

  cellSet.PrepareToAddCells(1, 4);
  cellSet.AddCell(svtkm::CELL_SHAPE_TETRA, 4, ids);
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet5()
{
  svtkm::cont::DataSet dataSet;

  const int nVerts = 11;
  using CoordType = svtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),     //0
    CoordType(1, 0, 0),     //1
    CoordType(1, 0, 1),     //2
    CoordType(0, 0, 1),     //3
    CoordType(0, 1, 0),     //4
    CoordType(1, 1, 0),     //5
    CoordType(1, 1, 1),     //6
    CoordType(0, 1, 1),     //7
    CoordType(2, 0.5, 0.5), //8
    CoordType(0, 2, 0),     //9
    CoordType(1, 2, 0)      //10
  };
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f,
                                 70.2f, 80.3f, 90.f,  10.f,  11.f };

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", svtkm::cont::Field::Association::POINTS, vars, nVerts, svtkm::CopyFlag::On));

  //Set cell scalar
  const int nCells = 4;
  svtkm::Float32 cellvar[nCells] = { 100.1f, 110.f, 120.2f, 130.5f };
  dataSet.AddField(make_Field(
    "cellvar", svtkm::cont::Field::Association::CELL_SET, cellvar, nCells, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  svtkm::Vec<svtkm::Id, 8> ids;

  cellSet.PrepareToAddCells(nCells, 23);

  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 5;
  ids[3] = 4;
  ids[4] = 3;
  ids[5] = 2;
  ids[6] = 6;
  ids[7] = 7;
  cellSet.AddCell(svtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);

  ids[0] = 1;
  ids[1] = 5;
  ids[2] = 6;
  ids[3] = 2;
  ids[4] = 8;
  cellSet.AddCell(svtkm::CELL_SHAPE_PYRAMID, 5, ids);

  ids[0] = 5;
  ids[1] = 8;
  ids[2] = 10;
  ids[3] = 6;
  cellSet.AddCell(svtkm::CELL_SHAPE_TETRA, 4, ids);

  ids[0] = 4;
  ids[1] = 7;
  ids[2] = 9;
  ids[3] = 5;
  ids[4] = 6;
  ids[5] = 10;
  cellSet.AddCell(svtkm::CELL_SHAPE_WEDGE, 6, ids);

  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet6()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(0);
  conn.push_back(1);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(4);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(6);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(0);
  conn.push_back(7);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 57.0f };
  svtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetZoo()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  constexpr int nVerts = 30;
  constexpr int nCells = 25;
  using CoordType = svtkm::Vec3f_32;

  std::vector<CoordType> coords =

    { { 0.00f, 0.00f, 0.00f }, { 1.00f, 0.00f, 0.00f }, { 2.00f, 0.00f, 0.00f },
      { 0.00f, 0.00f, 1.00f }, { 1.00f, 0.00f, 1.00f }, { 2.00f, 0.00f, 1.00f },
      { 0.00f, 1.00f, 0.00f }, { 1.00f, 1.00f, 0.00f }, { 2.00f, 1.00f, 0.00f },
      { 0.00f, 1.00f, 1.00f }, { 1.00f, 1.00f, 1.00f }, { 2.00f, 1.00f, 1.00f },
      { 0.00f, 2.00f, 0.00f }, { 1.00f, 2.00f, 0.00f }, { 2.00f, 2.00f, 0.00f },
      { 0.00f, 2.00f, 1.00f }, { 1.00f, 2.00f, 1.00f }, { 2.00f, 2.00f, 1.00f },
      { 1.00f, 3.00f, 1.00f }, { 2.75f, 0.00f, 1.00f }, { 3.00f, 0.00f, 0.75f },
      { 3.00f, 0.25f, 1.00f }, { 3.00f, 1.00f, 1.00f }, { 3.00f, 1.00f, 0.00f },
      { 2.57f, 2.00f, 1.00f }, { 3.00f, 1.75f, 1.00f }, { 3.00f, 1.75f, 0.75f },
      { 3.00f, 0.00f, 0.00f }, { 2.57f, 0.42f, 0.57f }, { 2.59f, 1.43f, 0.71f } };

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;

  shapes.push_back(svtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(4);
  conn.push_back(1);
  conn.push_back(6);
  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(7);

  shapes.push_back(svtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(1);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(2);
  conn.push_back(7);
  conn.push_back(10);
  conn.push_back(11);
  conn.push_back(8);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(23);
  conn.push_back(26);
  conn.push_back(24);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(24);
  conn.push_back(26);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(11);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(17);
  conn.push_back(24);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(24);
  conn.push_back(17);
  conn.push_back(8);
  conn.push_back(23);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(8);
  conn.push_back(11);
  conn.push_back(22);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(25);
  conn.push_back(22);
  conn.push_back(11);
  conn.push_back(17);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(26);
  conn.push_back(23);
  conn.push_back(22);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(8);
  conn.push_back(2);
  conn.push_back(27);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(22);
  conn.push_back(11);
  conn.push_back(8);
  conn.push_back(23);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(11);
  conn.push_back(5);
  conn.push_back(2);
  conn.push_back(8);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(21);
  conn.push_back(19);
  conn.push_back(5);
  conn.push_back(11);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(11);
  conn.push_back(22);
  conn.push_back(21);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(5);
  conn.push_back(19);
  conn.push_back(20);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(27);
  conn.push_back(20);
  conn.push_back(21);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(20);
  conn.push_back(27);
  conn.push_back(2);
  conn.push_back(5);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(19);
  conn.push_back(21);
  conn.push_back(20);
  conn.push_back(28);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(7);
  conn.push_back(6);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(6);
  conn.push_back(9);
  conn.push_back(15);
  conn.push_back(12);
  conn.push_back(16);

  shapes.push_back(svtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(10);
  conn.push_back(9);
  conn.push_back(16);

  shapes.push_back(svtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(12);
  conn.push_back(15);
  conn.push_back(16);
  conn.push_back(18);

  shapes.push_back(svtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(8);
  conn.push_back(14);
  conn.push_back(17);
  conn.push_back(7);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(svtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(11);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(10);
  conn.push_back(7);
  conn.push_back(16);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] =

    { 4.0,  5.0f, 9.5f, 5.5f, 6.0f, 9.5f, 5.0f, 5.5f, 5.7f, 6.5f, 6.4f, 6.9f, 6.6f, 6.1f, 7.1f,
      7.2f, 7.3f, 7.4f, 9.1f, 9.2f, 9.3f, 5.4f, 9.5f, 9.6f, 6.7f, 9.8f, 6.0f, 4.3f, 4.9f, 4.1f };

  svtkm::Float32 cellvar[nCells] =

    { 4.0f, 5.0f, 9.5f, 5.5f, 6.0f, 9.5f, 5.0f, 5.5f, 5.7f, 6.5f, 6.4f, 6.9f, 6.6f,
      6.1f, 7.1f, 7.2f, 7.3f, 7.4f, 9.1f, 9.2f, 9.3f, 5.4f, 9.5f, 9.6f, 6.7f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet7()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;

  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;


  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(0);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(1);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(2);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(4);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(6);

  shapes.push_back(svtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(7);


  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 10.f, 20.f, 33.f, 52.f };
  svtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet8()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 10;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;

  //I need two triangles because the leaf needs four nodes otherwise segfault?
  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(0);
  conn.push_back(1);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(1);
  conn.push_back(2);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(4);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(5);
  conn.push_back(6);

  shapes.push_back(svtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(6);
  conn.push_back(7);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(5);
  conn.push_back(4);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 57.0f };
  svtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);
  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetPolygonal()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 0.000f, 0.146f, -0.854f },   { 0.000f, 0.854f, -0.146f },
                                    { 0.707f, 0.354f, 0.354f },    { 0.707f, -0.354f, -0.354f } };

  // Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numindices;
  std::vector<svtkm::Id> conn;

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(1);

  shapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(4);
  conn.push_back(7);
  conn.push_back(1);

  shapes.push_back(svtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  shapes.push_back(svtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(5);
  conn.push_back(4);

  shapes.push_back(svtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(4);
  conn.push_back(1);
  conn.push_back(7);
  conn.push_back(6);
  conn.push_back(2);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  svtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 33.0f };
  svtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline svtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetCowNose()
{
  // prepare data array
  const int nVerts = 17;
  using CoordType = svtkm::Vec3f_64;
  CoordType coordinates[nVerts] = {
    CoordType(0.0480879, 0.151874, 0.107334),     CoordType(0.0293568, 0.245532, 0.125337),
    CoordType(0.0224398, 0.246495, 0.1351),       CoordType(0.0180085, 0.20436, 0.145316),
    CoordType(0.0307091, 0.152142, 0.0539249),    CoordType(0.0270341, 0.242992, 0.107567),
    CoordType(0.000684071, 0.00272505, 0.175648), CoordType(0.00946217, 0.077227, 0.187097),
    CoordType(-0.000168991, 0.0692243, 0.200755), CoordType(-0.000129414, 0.00247137, 0.176561),
    CoordType(0.0174172, 0.137124, 0.124553),     CoordType(0.00325994, 0.0797155, 0.184912),
    CoordType(0.00191765, 0.00589327, 0.16608),   CoordType(0.0174716, 0.0501928, 0.0930275),
    CoordType(0.0242103, 0.250062, 0.126256),     CoordType(0.0108188, 0.152774, 0.167914),
    CoordType(5.41687e-05, 0.00137834, 0.175119)
  };
  const int connectivitySize = 57;
  svtkm::Id pointId[connectivitySize] = { 0, 1, 3,  2, 3,  1, 4,  5,  0,  1, 0,  5,  7,  8,  6,
                                         9, 6, 8,  0, 10, 7, 11, 7,  10, 0, 6,  13, 12, 13, 6,
                                         1, 5, 14, 1, 14, 2, 0,  3,  15, 0, 13, 4,  6,  16, 12,
                                         6, 9, 16, 7, 11, 8, 0,  15, 10, 7, 6,  0 };

  // create DataSet
  svtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, svtkm::CopyFlag::On));

  svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
  connectivity.Allocate(connectivitySize);

  for (svtkm::Id i = 0; i < connectivitySize; ++i)
  {
    connectivity.GetPortalControl().Set(i, pointId[i]);
  }
  svtkm::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(nVerts, svtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);
  dataSet.SetCellSet(cellSet);

  std::vector<svtkm::Float32> pointvar(nVerts);
  std::iota(pointvar.begin(), pointvar.end(), 15.f);
  std::vector<svtkm::Float32> cellvar(connectivitySize / 3);
  std::iota(cellvar.begin(), cellvar.end(), 132.f);

  svtkm::cont::ArrayHandle<svtkm::Vec3f> pointvec;
  pointvec.Allocate(nVerts);
  SetPortal(pointvec.GetPortalControl());

  svtkm::cont::ArrayHandle<svtkm::Vec3f> cellvec;
  cellvec.Allocate(connectivitySize / 3);
  SetPortal(cellvec.GetPortalControl());

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", pointvar);
  dsf.AddCellField(dataSet, "cellvar", cellvar);
  dsf.AddPointField(dataSet, "point_vectors", pointvec);
  dsf.AddCellField(dataSet, "cell_vectors", cellvec);

  return dataSet;
}
}
}
} // namespace svtkm::cont::testing

#endif //svtk_m_cont_testing_MakeTestDataSet_h
