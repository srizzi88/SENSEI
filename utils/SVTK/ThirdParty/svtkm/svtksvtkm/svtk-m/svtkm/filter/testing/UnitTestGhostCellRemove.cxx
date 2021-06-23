//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/GhostCellRemove.h>

#include <svtkm/io/writer/SVTKDataSetWriter.h>


namespace
{

static svtkm::cont::ArrayHandle<svtkm::UInt8> StructuredGhostCellArray(svtkm::Id nx,
                                                                     svtkm::Id ny,
                                                                     svtkm::Id nz,
                                                                     int numLayers,
                                                                     bool addMidGhost = false)
{
  svtkm::Id numCells = nx * ny;
  if (nz > 0)
    numCells *= nz;

  constexpr svtkm::UInt8 normalCell = svtkm::CellClassification::NORMAL;
  constexpr svtkm::UInt8 duplicateCell = svtkm::CellClassification::GHOST;

  svtkm::cont::ArrayHandle<svtkm::UInt8> ghosts;
  ghosts.Allocate(numCells);
  auto portal = ghosts.GetPortalControl();
  for (svtkm::Id i = 0; i < numCells; i++)
  {
    if (numLayers == 0)
      portal.Set(i, normalCell);
    else
      portal.Set(i, duplicateCell);
  }

  if (numLayers > 0)
  {
    //2D case
    if (nz == 0)
    {
      for (svtkm::Id i = numLayers; i < nx - numLayers; i++)
        for (svtkm::Id j = numLayers; j < ny - numLayers; j++)
          portal.Set(j * nx + i, normalCell);
    }
    else
    {
      for (svtkm::Id i = numLayers; i < nx - numLayers; i++)
        for (svtkm::Id j = numLayers; j < ny - numLayers; j++)
          for (svtkm::Id k = numLayers; k < nz - numLayers; k++)
            portal.Set(k * nx * ny + j * nx + i, normalCell);
    }
  }

  if (addMidGhost)
  {
    if (nz == 0)
    {
      svtkm::Id mi = numLayers + (nx - numLayers) / 2;
      svtkm::Id mj = numLayers + (ny - numLayers) / 2;
      portal.Set(mj * nx + mi, duplicateCell);
    }
    else
    {
      svtkm::Id mi = numLayers + (nx - numLayers) / 2;
      svtkm::Id mj = numLayers + (ny - numLayers) / 2;
      svtkm::Id mk = numLayers + (nz - numLayers) / 2;
      portal.Set(mk * nx * ny + mj * nx + mi, duplicateCell);
    }
  }
  return ghosts;
}

static svtkm::cont::DataSet MakeUniform(svtkm::Id numI,
                                       svtkm::Id numJ,
                                       svtkm::Id numK,
                                       int numLayers,
                                       bool addMidGhost = false)
{
  svtkm::cont::DataSetBuilderUniform dsb;
  svtkm::cont::DataSet ds;

  if (numK == 0)
    ds = dsb.Create(svtkm::Id2(numI + 1, numJ + 1));
  else
    ds = dsb.Create(svtkm::Id3(numI + 1, numJ + 1, numK + 1));
  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers, addMidGhost);

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "svtkmGhostCells", ghosts);

  return ds;
}

static svtkm::cont::DataSet MakeRectilinear(svtkm::Id numI,
                                           svtkm::Id numJ,
                                           svtkm::Id numK,
                                           int numLayers,
                                           bool addMidGhost = false)
{
  svtkm::cont::DataSetBuilderRectilinear dsb;
  svtkm::cont::DataSet ds;
  std::size_t nx(static_cast<std::size_t>(numI + 1));
  std::size_t ny(static_cast<std::size_t>(numJ + 1));

  std::vector<float> x(nx), y(ny);
  for (std::size_t i = 0; i < nx; i++)
    x[i] = static_cast<float>(i);
  for (std::size_t i = 0; i < ny; i++)
    y[i] = static_cast<float>(i);

  if (numK == 0)
    ds = dsb.Create(x, y);
  else
  {
    std::size_t nz(static_cast<std::size_t>(numK + 1));
    std::vector<float> z(nz);
    for (std::size_t i = 0; i < nz; i++)
      z[i] = static_cast<float>(i);
    ds = dsb.Create(x, y, z);
  }

  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers, addMidGhost);

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "svtkmGhostCells", ghosts);

  return ds;
}

template <class CellSetType, svtkm::IdComponent NDIM>
static void MakeExplicitCells(const CellSetType& cellSet,
                              svtkm::Vec<svtkm::Id, NDIM>& dims,
                              svtkm::cont::ArrayHandle<svtkm::IdComponent>& numIndices,
                              svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
                              svtkm::cont::ArrayHandle<svtkm::Id>& conn)
{
  using Connectivity = svtkm::internal::ConnectivityStructuredInternals<NDIM>;

  svtkm::Id nCells = cellSet.GetNumberOfCells();
  svtkm::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(dims);

  svtkm::Id idx = 0;
  for (svtkm::Id i = 0; i < nCells; i++)
  {
    auto ptIds = structured.GetPointsOfCell(i);
    for (svtkm::IdComponent j = 0; j < NDIM; j++, idx++)
      conn.GetPortalControl().Set(idx, ptIds[j]);

    shapes.GetPortalControl().Set(
      i, (NDIM == 4 ? svtkm::CELL_SHAPE_QUAD : svtkm::CELL_SHAPE_HEXAHEDRON));
    numIndices.GetPortalControl().Set(i, NDIM);
  }
}

static svtkm::cont::DataSet MakeExplicit(svtkm::Id numI, svtkm::Id numJ, svtkm::Id numK, int numLayers)
{
  using CoordType = svtkm::Vec3f_32;

  svtkm::cont::DataSet dsUniform = MakeUniform(numI, numJ, numK, numLayers);

  auto coordData = dsUniform.GetCoordinateSystem(0).GetData();
  svtkm::Id numPts = coordData.GetNumberOfValues();
  svtkm::cont::ArrayHandle<CoordType> explCoords;

  explCoords.Allocate(numPts);
  auto explPortal = explCoords.GetPortalControl();
  auto cp = coordData.GetPortalConstControl();
  for (svtkm::Id i = 0; i < numPts; i++)
    explPortal.Set(i, cp.Get(i));

  svtkm::cont::DynamicCellSet cellSet = dsUniform.GetCellSet();
  svtkm::cont::ArrayHandle<svtkm::Id> conn;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;
  svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
  svtkm::cont::DataSet ds;
  svtkm::cont::DataSetBuilderExplicit dsb;

  if (cellSet.IsType<svtkm::cont::CellSetStructured<2>>())
  {
    svtkm::Id2 dims(numI, numJ);
    MakeExplicitCells(
      cellSet.Cast<svtkm::cont::CellSetStructured<2>>(), dims, numIndices, shapes, conn);
    ds = dsb.Create(explCoords, svtkm::CellShapeTagQuad(), 4, conn, "coordinates");
  }
  else if (cellSet.IsType<svtkm::cont::CellSetStructured<3>>())
  {
    svtkm::Id3 dims(numI, numJ, numK);
    MakeExplicitCells(
      cellSet.Cast<svtkm::cont::CellSetStructured<3>>(), dims, numIndices, shapes, conn);
    ds = dsb.Create(explCoords, svtkm::CellShapeTagHexahedron(), 8, conn, "coordinates");
  }

  auto ghosts = StructuredGhostCellArray(numI, numJ, numK, numLayers);

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "svtkmGhostCells", ghosts);

  return ds;
}

void TestGhostCellRemove()
{
  // specify some 2d tests: {numI, numJ, numK, numGhostLayers}.
  std::vector<std::vector<svtkm::Id>> tests2D = { { 4, 4, 0, 2 },  { 5, 5, 0, 2 },  { 10, 10, 0, 3 },
                                                 { 10, 5, 0, 2 }, { 5, 10, 0, 2 }, { 20, 10, 0, 3 },
                                                 { 10, 20, 0, 3 } };
  std::vector<std::vector<svtkm::Id>> tests3D = { { 4, 4, 4, 2 },    { 5, 5, 5, 2 },
                                                 { 10, 10, 10, 3 }, { 10, 5, 10, 2 },
                                                 { 5, 10, 10, 2 },  { 20, 10, 10, 3 },
                                                 { 10, 20, 10, 3 } };

  std::vector<std::vector<svtkm::Id>> tests;

  tests.insert(tests.end(), tests2D.begin(), tests2D.end());
  tests.insert(tests.end(), tests3D.begin(), tests3D.end());

  for (auto& t : tests)
  {
    svtkm::Id nx = t[0], ny = t[1], nz = t[2];
    int nghost = static_cast<int>(t[3]);
    for (int layer = 0; layer < nghost; layer++)
    {
      std::vector<std::string> dsTypes = { "uniform", "rectilinear", "explicit" };
      for (auto& dsType : dsTypes)
      {
        svtkm::cont::DataSet ds;
        if (dsType == "uniform")
          ds = MakeUniform(nx, ny, nz, layer);
        else if (dsType == "rectilinear")
          ds = MakeRectilinear(nx, ny, nz, layer);
        else if (dsType == "explicit")
          ds = MakeExplicit(nx, ny, nz, layer);

        std::vector<std::string> removeType = { "all", "byType" };
        for (auto& rt : removeType)
        {
          svtkm::filter::GhostCellRemove ghostCellRemoval;
          ghostCellRemoval.RemoveGhostField();

          if (rt == "all")
            ghostCellRemoval.RemoveAllGhost();
          else if (rt == "byType")
            ghostCellRemoval.RemoveByType(svtkm::CellClassification::GHOST);

          auto output = ghostCellRemoval.Execute(ds);
          svtkm::Id numCells = output.GetNumberOfCells();

          //Validate the output.

          svtkm::Id numCellsReq = (nx - 2 * layer) * (ny - 2 * layer);
          if (nz != 0)
            numCellsReq *= (nz - 2 * layer);

          SVTKM_TEST_ASSERT(numCellsReq == numCells, "Wrong number of cells in output");
          if (dsType == "uniform" || dsType == "rectilinear")
          {
            if (nz == 0)
            {
              SVTKM_TEST_ASSERT(output.GetCellSet().IsSameType(svtkm::cont::CellSetStructured<2>()),
                               "Wrong cell type for explicit conversion");
            }
            else if (nz > 0)
            {
              SVTKM_TEST_ASSERT(output.GetCellSet().IsSameType(svtkm::cont::CellSetStructured<3>()),
                               "Wrong cell type for explicit conversion");
            }
          }
          else
          {
            SVTKM_TEST_ASSERT(output.GetCellSet().IsType<svtkm::cont::CellSetExplicit<>>(),
                             "Wrong cell type for explicit conversion");
          }
        }

        // For structured, test the case where we have a ghost in the 'middle' of the cells.
        // This will produce an explicit cellset.
        if (dsType == "uniform" || dsType == "rectilinear")
        {
          if (dsType == "uniform")
            ds = MakeUniform(nx, ny, nz, layer, true);
          else if (dsType == "rectilinear")
            ds = MakeRectilinear(nx, ny, nz, layer, true);

          svtkm::filter::GhostCellRemove ghostCellRemoval;
          ghostCellRemoval.RemoveGhostField();
          auto output = ghostCellRemoval.Execute(ds);
          SVTKM_TEST_ASSERT(output.GetCellSet().IsType<svtkm::cont::CellSetExplicit<>>(),
                           "Wrong cell type for explicit conversion");
        }
      }
    }
  }
}
}

int UnitTestGhostCellRemove(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestGhostCellRemove, argc, argv);
}
