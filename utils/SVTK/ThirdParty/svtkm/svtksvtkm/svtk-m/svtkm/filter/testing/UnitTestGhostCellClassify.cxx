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

#include <svtkm/filter/GhostCellClassify.h>

namespace
{

static svtkm::cont::DataSet MakeUniform(svtkm::Id numI, svtkm::Id numJ, svtkm::Id numK)
{
  svtkm::cont::DataSetBuilderUniform dsb;

  svtkm::cont::DataSet ds;

  if (numJ == 0 && numK == 0)
    ds = dsb.Create(numI + 1);
  else if (numK == 0)
    ds = dsb.Create(svtkm::Id2(numI + 1, numJ + 1));
  else
    ds = dsb.Create(svtkm::Id3(numI + 1, numJ + 1, numK + 1));

  return ds;
}

static svtkm::cont::DataSet MakeRectilinear(svtkm::Id numI, svtkm::Id numJ, svtkm::Id numK)
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

  return ds;
}

void TestStructured()
{
  std::cout << "Testing ghost cells for structured datasets." << std::endl;

  // specify some 2d tests: {numI, numJ, numK, numGhostLayers}.
  std::vector<std::vector<svtkm::Id>> tests1D = {
    { 8, 0, 0, 1 }, { 5, 0, 0, 1 }, { 10, 0, 0, 1 }, { 20, 0, 0, 1 }
  };
  std::vector<std::vector<svtkm::Id>> tests2D = { { 8, 4, 0, 1 },  { 5, 5, 0, 1 },  { 10, 10, 0, 1 },
                                                 { 10, 5, 0, 1 }, { 5, 10, 0, 1 }, { 20, 10, 0, 1 },
                                                 { 10, 20, 0, 1 } };
  std::vector<std::vector<svtkm::Id>> tests3D = {
    { 8, 8, 10, 1 },   { 5, 5, 5, 1 },    { 10, 10, 10, 1 },    { 10, 5, 10, 1 },  { 5, 10, 10, 1 },
    { 20, 10, 10, 1 }, { 10, 20, 10, 1 }, { 128, 128, 128, 1 }, { 256, 64, 10, 1 }
  };

  std::vector<std::vector<svtkm::Id>> tests;

  tests.insert(tests.end(), tests1D.begin(), tests1D.end());
  tests.insert(tests.end(), tests2D.begin(), tests2D.end());
  tests.insert(tests.end(), tests3D.begin(), tests3D.end());

  for (auto& t : tests)
  {
    svtkm::Id nx = t[0], ny = t[1], nz = t[2];
    svtkm::Id nghost = t[3];
    for (svtkm::Id layer = 1; layer <= nghost; layer++)
    {
      svtkm::cont::DataSet ds;
      std::vector<std::string> dsTypes = { "uniform", "rectilinear" };

      for (auto& dsType : dsTypes)
      {
        if (dsType == "uniform")
          ds = MakeUniform(nx, ny, nz);
        else if (dsType == "rectilinear")
          ds = MakeRectilinear(nx, ny, nz);

        svtkm::filter::GhostCellClassify addGhost;

        auto output = addGhost.Execute(ds, svtkm::filter::GhostCellClassifyPolicy());

        //Validate the output.
        SVTKM_TEST_ASSERT(output.HasCellField("svtkmGhostCells"),
                         "Ghost cells array not found in output");
        svtkm::Id numCells = output.GetNumberOfCells();
        auto fieldArray = output.GetCellField("svtkmGhostCells").GetData();
        SVTKM_TEST_ASSERT(fieldArray.GetNumberOfValues() == numCells,
                         "Wrong number of values in ghost cell array");

        //Check the number of normal cells.
        svtkm::cont::ArrayHandle<svtkm::UInt8> ghostArray;
        fieldArray.CopyTo(ghostArray);

        svtkm::Id numNormalCells = 0;
        auto portal = ghostArray.GetPortalConstControl();
        constexpr svtkm::UInt8 normalCell = svtkm::CellClassification::NORMAL;
        for (svtkm::Id i = 0; i < numCells; i++)
          if (portal.Get(i) == normalCell)
            numNormalCells++;

        svtkm::Id requiredNumNormalCells = (nx - 2 * layer);
        if (ny > 0)
          requiredNumNormalCells *= (ny - 2 * layer);
        if (nz > 0)
          requiredNumNormalCells *= (nz - 2 * layer);
        SVTKM_TEST_ASSERT(requiredNumNormalCells == numNormalCells, "Wrong number of normal cells");
      }
    }
  }
}

void TestGhostCellClassify()
{
  TestStructured();
}
}

int UnitTestGhostCellClassify(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestGhostCellClassify, argc, argv);
}
