//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/ZFPCompressor.h>
#include <svtkm/worklet/ZFPDecompress.h>

#include <svtkm/worklet/ZFP1DCompressor.h>
#include <svtkm/worklet/ZFP1DDecompress.h>

#include <svtkm/worklet/ZFP2DCompressor.h>
#include <svtkm/worklet/ZFP2DDecompress.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/zfp/ZFPTools.h>

#include <algorithm>
#include <fstream>
#include <iostream>


template <typename T>
void writeArray(svtkm::cont::ArrayHandle<T>& field, std::string filename)
{
  auto val = svtkm::worklet::zfp::detail::GetSVTKMPointer(field);
  std::ofstream output(filename, std::ios::binary | std::ios::out);
  output.write(reinterpret_cast<char*>(val), field.GetNumberOfValues() * 8);
  output.close();

  for (int i = 0; i < field.GetNumberOfValues(); i++)
  {
    std::cout << val[i] << " ";
  }
  std::cout << std::endl;
}

using Handle64 = svtkm::cont::ArrayHandle<svtkm::Float64>;
template <typename Scalar>
void Test1D(int rate)
{
  std::cout << "Testing ZFP 1d:" << std::endl;
  svtkm::Id dims = 256;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make1DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();

  svtkm::worklet::ZFP1DCompressor compressor;
  svtkm::worklet::ZFP1DDecompressor decompressor;

  if (svtkm::cont::IsType<Handle64>(dynField))
  {
    svtkm::cont::ArrayHandle<Scalar> handle;
    const svtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (svtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);
    //writeArray(compressed, "output.zfp");
    svtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}
template <typename Scalar>
void Test2D(int rate)
{
  std::cout << "Testing ZFP 2d:" << std::endl;
  svtkm::Id2 dims(16, 16);
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make2DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();

  svtkm::worklet::ZFP2DCompressor compressor;
  svtkm::worklet::ZFP2DDecompressor decompressor;

  if (svtkm::cont::IsType<Handle64>(dynField))
  {
    svtkm::cont::ArrayHandle<Scalar> handle;
    const svtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (svtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);
    svtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}
template <typename Scalar>
void Test3D(int rate)
{
  std::cout << "Testing ZFP 3d:" << std::endl;
  svtkm::Id3 dims(4, 4, 4);
  //svtkm::Id3 dims(4,4,7);
  //svtkm::Id3 dims(8,8,8);
  //svtkm::Id3 dims(256,256,256);
  //svtkm::Id3 dims(128,128,128);
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();

  svtkm::worklet::ZFPCompressor compressor;
  svtkm::worklet::ZFPDecompressor decompressor;

  if (svtkm::cont::IsType<Handle64>(dynField))
  {
    svtkm::cont::ArrayHandle<Scalar> handle;
    const svtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (svtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);

    svtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}

void TestZFP()
{
  Test3D<svtkm::Float64>(4);
  Test2D<svtkm::Float64>(4);
  Test1D<svtkm::Float64>(4);
  //Test3D<svtkm::Float32>(4);
  //Test3D<svtkm::Int64>(4);
  //Test3D<svtkm::Int32>(4);
}

int UnitTestZFPCompressor(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestZFP, argc, argv);
}
