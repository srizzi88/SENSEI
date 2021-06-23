//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/raytracing/ChannelBuffer.h>
#include <svtkm/rendering/raytracing/ChannelBufferOperations.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <svtkm/worklet/DispatcherMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class BufferAddition : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  BufferAddition() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  SVTKM_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 += value1;
  }
}; //class BufferAddition

class BufferMultiply : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  BufferMultiply() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  SVTKM_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 *= value1;
  }
}; //class BufferMultiply

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer()
{
  this->NumChannels = 4;
  this->Size = 0;
  this->Name = "default";
}

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer(const svtkm::Int32 numChannels, const svtkm::Id size)
{
  if (size < 0)
    throw svtkm::cont::ErrorBadValue("ChannelBuffer: Size must be greater that -1");
  if (numChannels < 0)
    throw svtkm::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that -1");

  this->NumChannels = numChannels;
  this->Size = size;

  Buffer.Allocate(this->Size * this->NumChannels);
}

template <typename Precision>
svtkm::Int32 ChannelBuffer<Precision>::GetNumChannels() const
{
  return this->NumChannels;
}

template <typename Precision>
svtkm::Id ChannelBuffer<Precision>::GetSize() const
{
  return this->Size;
}

template <typename Precision>
svtkm::Id ChannelBuffer<Precision>::GetBufferLength() const
{
  return this->Size * static_cast<svtkm::Id>(this->NumChannels);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetName(const std::string name)
{
  this->Name = name;
}

template <typename Precision>
std::string ChannelBuffer<Precision>::GetName() const
{
  return this->Name;
}


template <typename Precision>
void ChannelBuffer<Precision>::AddBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw svtkm::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw svtkm::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  svtkm::worklet::DispatcherMapField<BufferAddition>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::MultiplyBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw svtkm::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw svtkm::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  svtkm::worklet::DispatcherMapField<BufferMultiply>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::Resize(const svtkm::Id newSize)
{
  if (newSize < 0)
    throw svtkm::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1");
  this->Size = newSize;
  this->Buffer.Allocate(this->Size * static_cast<svtkm::Id>(NumChannels));
}

class ExtractChannel : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id NumChannels; // the nnumber of channels in the buffer
  svtkm::Id ChannelNum;  // the channel to extract

public:
  SVTKM_CONT
  ExtractChannel(const svtkm::Int32 numChannels, const svtkm::Int32 channel)
    : NumChannels(numChannels)
    , ChannelNum(channel)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename T, typename BufferPortalType>
  SVTKM_EXEC void operator()(T& outValue,
                            const BufferPortalType& inBuffer,
                            const svtkm::Id& index) const
  {
    svtkm::Id valueIndex = index * NumChannels + ChannelNum;
    BOUNDS_CHECK(inBuffer, valueIndex);
    outValue = inBuffer.Get(valueIndex);
  }
}; //class Extract Channel

template <typename Precision>
struct ExtractChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  svtkm::cont::ArrayHandle<Precision> Output;
  svtkm::Int32 Channel;

  ExtractChannelFunctor(ChannelBuffer<Precision>* self,
                        svtkm::cont::ArrayHandle<Precision> output,
                        const svtkm::Int32 channel)
    : Self(self)
    , Output(output)
    , Channel(channel)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    Output.PrepareForOutput(Self->GetSize(), device);
    svtkm::worklet::DispatcherMapField<ExtractChannel> dispatcher(
      ExtractChannel(Self->GetNumChannels(), Channel));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Output, Self->Buffer);
    return true;
  }
};

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::GetChannel(const svtkm::Int32 channel)
{
  if (channel < 0 || channel >= this->NumChannels)
    throw svtkm::cont::ErrorBadValue("ChannelBuffer: invalid channel to extract");
  ChannelBuffer<Precision> output(1, this->Size);
  output.SetName(this->Name);
  if (this->Size == 0)
  {
    return output;
  }
  ExtractChannelFunctor<Precision> functor(this, output.Buffer, channel);

  svtkm::cont::TryExecute(functor);
  return output;
}

//static
class Expand : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Int32 NumChannels;

public:
  SVTKM_CONT
  Expand(const svtkm::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);
  template <typename T, typename IndexPortalType, typename BufferPortalType>
  SVTKM_EXEC void operator()(const T& inValue,
                            const IndexPortalType& sparseIndexes,
                            BufferPortalType& outBuffer,
                            const svtkm::Id& index) const
  {
    svtkm::Id sparse = index / NumChannels;
    BOUNDS_CHECK(sparseIndexes, sparse);
    svtkm::Id sparseIndex = sparseIndexes.Get(sparse) * NumChannels;
    svtkm::Id outIndex = sparseIndex + index % NumChannels;
    BOUNDS_CHECK(outBuffer, outIndex);
    outBuffer.Set(outIndex, inValue);
  }
}; //class Expand

template <typename Precision>
struct ExpandFunctorSignature
{
  svtkm::cont::ArrayHandle<Precision> Input;
  svtkm::cont::ArrayHandle<svtkm::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  svtkm::cont::ArrayHandle<Precision> Signature;
  svtkm::Id OutputLength;
  svtkm::Int32 NumChannels;


  ExpandFunctorSignature(svtkm::cont::ArrayHandle<Precision> input,
                         svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
                         ChannelBuffer<Precision>* outChannelBuffer,
                         svtkm::Id outputLength,
                         svtkm::Int32 numChannels,
                         svtkm::cont::ArrayHandle<Precision> signature)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , Signature(signature)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    svtkm::Id totalSize = OutputLength * static_cast<svtkm::Id>(NumChannels);
    Output->Buffer.PrepareForOutput(totalSize, device);
    ChannelBufferOperations::InitChannels(*Output, Signature, device);

    svtkm::worklet::DispatcherMapField<Expand> dispatcher((Expand(NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input, SparseIndexes, Output->Buffer);

    return true;
  }
};

template <typename Precision>
struct ExpandFunctor
{
  svtkm::cont::ArrayHandle<Precision> Input;
  svtkm::cont::ArrayHandle<svtkm::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  svtkm::Id OutputLength;
  svtkm::Int32 NumChannels;
  Precision InitVal;


  ExpandFunctor(svtkm::cont::ArrayHandle<Precision> input,
                svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
                ChannelBuffer<Precision>* outChannelBuffer,
                svtkm::Id outputLength,
                svtkm::Int32 numChannels,
                Precision initVal)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
    , InitVal(initVal)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    svtkm::Id totalSize = OutputLength * static_cast<svtkm::Id>(NumChannels);
    Output->Buffer.PrepareForOutput(totalSize, device);
    ChannelBufferOperations::InitConst(*Output, InitVal, device);

    svtkm::worklet::DispatcherMapField<Expand> dispatcher((Expand(NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input, SparseIndexes, Output->Buffer);

    return true;
  }
};

template <typename Precision>
class NormalizeBuffer : public svtkm::worklet::WorkletMapField
{
protected:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Invert;

public:
  SVTKM_CONT
  NormalizeBuffer(const Precision minScalar, const Precision maxScalar, bool invert)
    : MinScalar(minScalar)
    , Invert(invert)
  {
    if (maxScalar - minScalar == 0.)
    {
      InvDeltaScalar = MinScalar;
    }
    else
    {
      InvDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  SVTKM_EXEC
  void operator()(Precision& value) const
  {
    value = (value - MinScalar) * InvDeltaScalar;
    if (Invert)
      value = 1.f - value;
  }
}; //class normalize buffer


template <typename Precision>
struct NormalizeFunctor
{
  svtkm::cont::ArrayHandle<Precision> Input;
  bool Invert;

  NormalizeFunctor(svtkm::cont::ArrayHandle<Precision> input, bool invert)
    : Input(input)
    , Invert(invert)
  {
  }

  template <typename Device>
  bool operator()(Device svtkmNotUsed(device))
  {
    auto asField = svtkm::cont::make_FieldPoint("name meaningless", this->Input);
    svtkm::Range range;
    asField.GetRange(&range);
    Precision minScalar = static_cast<Precision>(range.Min);
    Precision maxScalar = static_cast<Precision>(range.Max);
    svtkm::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
      NormalizeBuffer<Precision>(minScalar, maxScalar, Invert));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input);

    return true;
  }
};


template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
  const svtkm::Id outputSize,
  svtkm::cont::ArrayHandle<Precision> signature)
{
  SVTKM_ASSERT(this->NumChannels == signature.GetPortalConstControl().GetNumberOfValues());
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctorSignature<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, signature);

  svtkm::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
  const svtkm::Id outputSize,
  Precision initValue)
{
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctor<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, initValue);

  svtkm::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
void ChannelBuffer<Precision>::Normalize(bool invert)
{

  auto asField = svtkm::cont::make_FieldPoint("name meaningless", this->Buffer);
  svtkm::Range range;
  asField.GetRange(&range);
  Precision minScalar = static_cast<Precision>(range.Min);
  Precision maxScalar = static_cast<Precision>(range.Max);
  svtkm::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
    NormalizeBuffer<Precision>(minScalar, maxScalar, invert));
  dispatcher.Invoke(this->Buffer);
}

template <typename Precision>
struct ResizeChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  svtkm::Int32 NumChannels;

  ResizeChannelFunctor(ChannelBuffer<Precision>* self, svtkm::Int32 numChannels)
    : Self(self)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    Self->SetNumChannels(NumChannels, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitConst(const Precision value)
{
  svtkm::cont::ArrayHandleConstant<Precision> valueHandle(value, this->GetBufferLength());
  svtkm::cont::Algorithm::Copy(valueHandle, this->Buffer);
}

template <typename Precision>
struct InitChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  const svtkm::cont::ArrayHandle<Precision>& Signature;
  InitChannelFunctor(ChannelBuffer<Precision>* self,
                     const svtkm::cont::ArrayHandle<Precision>& signature)
    : Self(self)
    , Signature(signature)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    ChannelBufferOperations::InitChannels(*Self, Signature, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitChannels(const svtkm::cont::ArrayHandle<Precision>& signature)
{
  InitChannelFunctor<Precision> functor(this, signature);
  svtkm::cont::TryExecute(functor);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetNumChannels(const svtkm::Int32 numChannels)
{
  ResizeChannelFunctor<Precision> functor(this, numChannels);
  svtkm::cont::TryExecute(functor);
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::Copy()
{
  ChannelBuffer res(NumChannels, Size);
  svtkm::cont::Algorithm::Copy(this->Buffer, res.Buffer);
  return res;
}

// Instantiate supported types
template class ChannelBuffer<svtkm::Float32>;
template class ChannelBuffer<svtkm::Float64>;
}
}
} // namespace svtkm::rendering::raytracing
