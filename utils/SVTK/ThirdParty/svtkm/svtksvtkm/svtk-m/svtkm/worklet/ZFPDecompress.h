//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_decompressor_h
#define svtk_m_worklet_zfp_decompressor_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/AtomicArray.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/zfp/ZFPDecode3.h>
#include <svtkm/worklet/zfp/ZFPTools.h>

using ZFPWord = svtkm::UInt64;

#include <stdio.h>

namespace svtkm
{
namespace worklet
{
class ZFPDecompressor
{
public:
  template <typename Scalar, typename StorageIn, typename StorageOut>
  void Decompress(const svtkm::cont::ArrayHandle<svtkm::Int64, StorageIn>& encodedData,
                  svtkm::cont::ArrayHandle<Scalar, StorageOut>& output,
                  const svtkm::Float64 requestedRate,
                  svtkm::Id3 dims)
  {
    //DataDumpb(data, "uncompressed");
    zfp::ZFPStream stream;
    const svtkm::Int32 topoDims = 3;
    ;
    stream.SetRate(requestedRate, topoDims, svtkm::Float64());


    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    svtkm::Id3 paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims[0] % 4 != 0)
      paddedDims[0] += 4 - dims[0] % 4;
    if (paddedDims[1] % 4 != 0)
      paddedDims[1] += 4 - dims[1] % 4;
    if (paddedDims[2] % 4 != 0)
      paddedDims[2] += 4 - dims[2] % 4;
    const svtkm::Id four = 4;
    svtkm::Id totalBlocks =
      (paddedDims[0] / four) * (paddedDims[1] / (four) * (paddedDims[2] / four));


    zfp::detail::CalcMem3d(paddedDims, stream.minbits);

    // hopefully this inits/allocates the mem only on the device
    output.Allocate(dims[0] * dims[1] * dims[2]);

    // launch 1 thread per zfp block
    svtkm::cont::ArrayHandleCounting<svtkm::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = svtkm::cont::Timer<svtkm::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    svtkm::worklet::DispatcherMapField<zfp::Decode3> decompressDispatcher(
      zfp::Decode3(dims, paddedDims, stream.maxbits));
    decompressDispatcher.Invoke(blockCounter, output, encodedData);

    //    svtkm::Float64 time = timer.GetElapsedTime();
    //    size_t total_bytes =  output.GetNumberOfValues() * sizeof(svtkm::Float64);
    //    svtkm::Float64 gB = svtkm::Float64(total_bytes) / (1024. * 1024. * 1024.);
    //    svtkm::Float64 rate = gB / time;
    //    std::cout<<"Decompress time "<<time<<" sec\n";
    //    std::cout<<"Decompress rate "<<rate<<" GB / sec\n";
    //    DataDump(output, "decompressed");
  }
};
} // namespace worklet
} // namespace svtkm
#endif //  svtk_m_worklet_zfp_compressor_h
