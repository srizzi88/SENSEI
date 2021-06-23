//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_compressor_h
#define svtk_m_worklet_zfp_compressor_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/AtomicArray.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/worklet/DispatcherMapField.h>

#include <svtkm/worklet/zfp/ZFPEncode3.h>
#include <svtkm/worklet/zfp/ZFPTools.h>

using ZFPWord = svtkm::UInt64;

#include <stdio.h>

namespace svtkm
{
namespace worklet
{

class ZFPCompressor
{
public:
  template <typename Scalar, typename Storage>
  svtkm::cont::ArrayHandle<svtkm::Int64> Compress(
    const svtkm::cont::ArrayHandle<Scalar, Storage>& data,
    const svtkm::Float64 requestedRate,
    const svtkm::Id3 dims)
  {
    // DataDump(data, "uncompressed");
    zfp::ZFPStream stream;
    const svtkm::Int32 topoDims = 3;
    stream.SetRate(requestedRate, topoDims, svtkm::Float64());
    //SVTKM_ASSERT(

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


    size_t outbits = zfp::detail::CalcMem3d(paddedDims, stream.minbits);
    svtkm::Id outsize = svtkm::Id(outbits / sizeof(ZFPWord));

    svtkm::cont::ArrayHandle<svtkm::Int64> output;
    // hopefully this inits/allocates the mem only on the device
    svtkm::cont::ArrayHandleConstant<svtkm::Int64> zero(0, outsize);
    svtkm::cont::Algorithm::Copy(zero, output);

    // launch 1 thread per zfp block
    svtkm::cont::ArrayHandleCounting<svtkm::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = svtkm::cont::Timer<svtkm::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    svtkm::worklet::DispatcherMapField<zfp::Encode3> compressDispatcher(
      zfp::Encode3(dims, paddedDims, stream.maxbits));
    compressDispatcher.Invoke(blockCounter, data, output);

    //    svtkm::Float64 time = timer.GetElapsedTime();
    //    size_t total_bytes =  data.GetNumberOfValues() * sizeof(svtkm::Float64);
    //    svtkm::Float64 gB = svtkm::Float64(total_bytes) / (1024. * 1024. * 1024.);
    //    svtkm::Float64 rate = gB / time;
    //    std::cout<<"Compress time "<<time<<" sec\n";
    //    std::cout<<"Compress rate "<<rate<<" GB / sec\n";
    //    DataDump(output, "compressed");

    return output;
  }
};
} // namespace worklet
} // namespace svtkm
#endif //  svtk_m_worklet_zfp_compressor_h
