//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/source/Tangle.h>

#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/Testing.h>

void TangleSourceTest()
{
  svtkm::cont::Timer timer;
  timer.Start();

  svtkm::source::Tangle source(svtkm::Id3{ 20, 20, 20 });
  svtkm::cont::DataSet ds = source.Execute();


  double time = timer.GetElapsedTime();

  std::cout << "Default wavelet took " << time << "s.\n";

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    SVTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    SVTKM_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // check the cell scalars
  {
    using ScalarHandleType = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;

    auto field = ds.GetCellField("cellvar");
    auto dynData = field.GetData();
    SVTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    SVTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 8000), "Incorrect number of elements.");

    for (svtkm::Id i = 0; i < 8000; ++i)
    {
      SVTKM_TEST_ASSERT(test_equal(data.Get(i), i), "Incorrect scalar value.");
    }
  }

  // Spot check some node scalars
  {
    using ScalarHandleType = svtkm::cont::ArrayHandle<svtkm::Float32>;

    auto field = ds.GetPointField("nodevar");
    auto dynData = field.GetData();
    SVTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    SVTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of scalars.");

    SVTKM_TEST_ASSERT(test_equal(data.Get(0), 24.46), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(16), 16.1195), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(21), 20.5988), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(256), 8.58544), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(1024), 1.56976), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(1987), 1.04074), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(2048), 0.95236), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(3110), 6.39556), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(4097), 2.62186), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(6599), 7.79722), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(7999), 7.94986), "Incorrect scalar value.");
  }
}

int UnitTestTangleSource(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TangleSourceTest, argc, argv);
}
