//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Math.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/ZFPCompressor1D.h>
#include <svtkm/filter/ZFPCompressor2D.h>
#include <svtkm/filter/ZFPCompressor3D.h>
#include <svtkm/filter/ZFPDecompressor1D.h>
#include <svtkm/filter/ZFPDecompressor2D.h>
#include <svtkm/filter/ZFPDecompressor3D.h>

namespace svtkm_ut_zfp_filter
{

void TestZFP1DFilter(svtkm::Float64 rate)
{


  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make1DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();
  svtkm::cont::ArrayHandle<svtkm::Float64> field =
    dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto oport = field.GetPortalControl();

  svtkm::filter::ZFPCompressor1D compressor;
  svtkm::filter::ZFPDecompressor1D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < field.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFP2DFilter(svtkm::Float64 rate)
{


  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make2DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();
  ;
  svtkm::cont::ArrayHandle<svtkm::Float64> field =
    dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto oport = field.GetPortalControl();


  svtkm::filter::ZFPCompressor2D compressor;
  svtkm::filter::ZFPDecompressor2D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFP3DFilter(svtkm::Float64 rate)
{


  const svtkm::Id3 dims(4, 4, 4);
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();
  svtkm::cont::ArrayHandle<svtkm::Float64> field =
    dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto oport = field.GetPortalControl();


  svtkm::filter::ZFPCompressor3D compressor;
  svtkm::filter::ZFPDecompressor3D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<svtkm::cont::ArrayHandle<svtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFPFilter()
{
  TestZFP1DFilter(4);
  TestZFP2DFilter(4);
  TestZFP2DFilter(4);
}
} // anonymous namespace

int UnitTestZFP(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(svtkm_ut_zfp_filter::TestZFPFilter, argc, argv);
}
