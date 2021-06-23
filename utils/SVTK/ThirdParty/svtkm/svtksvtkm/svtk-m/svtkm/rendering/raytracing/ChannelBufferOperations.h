//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_rendering_raytracing_ChannelBuffer_Operations_h
#define svtkm_rendering_raytracing_ChannelBuffer_Operations_h

#include <svtkm/Types.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCast.h>

#include <svtkm/rendering/raytracing/ChannelBuffer.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class CompactBuffer : public svtkm::worklet::WorkletMapField
{
protected:
  const svtkm::Id NumChannels; // the number of channels in the buffer

public:
  SVTKM_CONT
  CompactBuffer(const svtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  template <typename InBufferPortalType, typename OutBufferPortalType>
  SVTKM_EXEC void operator()(const svtkm::UInt8& mask,
                            const InBufferPortalType& inBuffer,
                            const svtkm::Id& offset,
                            OutBufferPortalType& outBuffer,
                            const svtkm::Id& index) const
  {
    if (mask == 0)
    {
      return;
    }
    svtkm::Id inIndex = index * NumChannels;
    svtkm::Id outIndex = offset * NumChannels;
    for (svtkm::Int32 i = 0; i < NumChannels; ++i)
    {
      BOUNDS_CHECK(inBuffer, inIndex + i);
      BOUNDS_CHECK(outBuffer, outIndex + i);
      outBuffer.Set(outIndex + i, inBuffer.Get(inIndex + i));
    }
  }
}; //class Compact

class InitBuffer : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Int32 NumChannels;

public:
  SVTKM_CONT
  InitBuffer(const svtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename ValueType, typename PortalType>
  SVTKM_EXEC void operator()(ValueType& outValue,
                            const PortalType& source,
                            const svtkm::Id& index) const
  {
    outValue = source.Get(index % NumChannels);
  }
}; //class InitBuffer


} // namespace detail

class ChannelBufferOperations
{
public:
  template <typename Precision>
  static void Compact(ChannelBuffer<Precision>& buffer,
                      svtkm::cont::ArrayHandle<UInt8>& masks,
                      const svtkm::Id& newSize)
  {
    svtkm::cont::ArrayHandle<svtkm::Id> offsets;
    offsets.Allocate(buffer.Size);
    svtkm::cont::ArrayHandleCast<svtkm::Id, svtkm::cont::ArrayHandle<svtkm::UInt8>> castedMasks(masks);
    svtkm::cont::Algorithm::ScanExclusive(castedMasks, offsets);

    svtkm::cont::ArrayHandle<Precision> compactedBuffer;
    compactedBuffer.Allocate(newSize * buffer.NumChannels);

    svtkm::worklet::DispatcherMapField<detail::CompactBuffer> dispatcher(
      detail::CompactBuffer(buffer.NumChannels));
    dispatcher.Invoke(masks, buffer.Buffer, offsets, compactedBuffer);
    buffer.Buffer = compactedBuffer;
    buffer.Size = newSize;
  }

  template <typename Device, typename Precision>
  static void InitChannels(ChannelBuffer<Precision>& buffer,
                           svtkm::cont::ArrayHandle<Precision> sourceSignature,
                           Device)
  {
    if (sourceSignature.GetNumberOfValues() != buffer.NumChannels)
    {
      std::string msg = "ChannelBuffer: number of bins in sourse signature must match NumChannels";
      throw svtkm::cont::ErrorBadValue(msg);
    }
    svtkm::worklet::DispatcherMapField<detail::InitBuffer> initBufferDispatcher(
      detail::InitBuffer(buffer.NumChannels));
    initBufferDispatcher.SetDevice(Device());
    initBufferDispatcher.Invoke(buffer.Buffer, sourceSignature);
  }

  template <typename Device, typename Precision>
  static void InitConst(ChannelBuffer<Precision>& buffer, const Precision value, Device)
  {
    svtkm::cont::ArrayHandleConstant<Precision> valueHandle(value, buffer.GetBufferLength());
    svtkm::cont::Algorithm::Copy(Device(), valueHandle, buffer.Buffer);
  }
};
}
}
} // namespace svtkm::rendering::raytracing
#endif
