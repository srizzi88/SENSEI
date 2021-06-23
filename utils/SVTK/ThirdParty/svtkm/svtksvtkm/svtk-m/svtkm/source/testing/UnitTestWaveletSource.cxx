//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/source/Wavelet.h>

#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/Testing.h>

void WaveletSourceTest()
{
  svtkm::cont::Timer timer;
  timer.Start();

  svtkm::source::Wavelet source;
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

  // Spot check some scalars
  {
    using ScalarHandleType = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;

    auto field = ds.GetPointField("scalars");
    auto dynData = field.GetData();
    SVTKM_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.Cast<ScalarHandleType>();
    auto data = handle.GetPortalConstControl();

    SVTKM_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of scalars.");

    SVTKM_TEST_ASSERT(test_equal(data.Get(0), 60.7635), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(16), 99.6115), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(21), 69.1968), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(256), 118.620), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(1024), 140.466), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(1987), 203.720), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(2048), 223.010), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(3110), 128.282), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(4097), 153.913), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(6599), 120.068), "Incorrect scalar value.");
    SVTKM_TEST_ASSERT(test_equal(data.Get(7999), 65.6710), "Incorrect scalar value.");
  }
}

int UnitTestWaveletSource(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(WaveletSourceTest, argc, argv);
}
