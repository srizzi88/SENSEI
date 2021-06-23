//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_rendering_raytracing_ChannelBuffer_h
#define svtkm_rendering_raytracing_ChannelBuffer_h

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>

#include <string>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
///
///  \brief Mananges a buffer that contains many channels per value (e.g., RGBA values).
///
///  \c The ChannelBuffer class is meant to handle a buffer of values with potentially many
///  channels. While RGBA values could be placed in a Vec<T,4>, data with a large number of
///  channels (e.g., 100+ energy bins) are better handled by a raw array. Rays can have color,
///  absorption, absorption + emission, or even track additional scalar values to support
///  standards such as Cinema. This class allows us to treat all of these different use cases
///  with the same type.
///
///  This class has methods that can be utilized by other SVTK-m classes that already have a
///  a device dapter specified, and can be used by external callers where the call executes
///  on a device through the try execute mechanism.
///
///  \c Currently, the supported types are floating point to match the precision of the rays.
///

template <typename Precision>
class SVTKM_RENDERING_EXPORT ChannelBuffer
{
protected:
  svtkm::Int32 NumChannels;
  svtkm::Id Size;
  std::string Name;
  friend class ChannelBufferOperations;

public:
  svtkm::cont::ArrayHandle<Precision> Buffer;

  /// Functions we want accessble outside of svtkm some of which execute
  /// on a device
  ///
  SVTKM_CONT
  ChannelBuffer();

  SVTKM_CONT
  ChannelBuffer(const svtkm::Int32 numChannels, const svtkm::Id size);

  SVTKM_CONT
  ChannelBuffer<Precision> GetChannel(const svtkm::Int32 channel);

  ChannelBuffer<Precision> ExpandBuffer(svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
                                        const svtkm::Id outputSize,
                                        svtkm::cont::ArrayHandle<Precision> signature);

  ChannelBuffer<Precision> ExpandBuffer(svtkm::cont::ArrayHandle<svtkm::Id> sparseIndexes,
                                        const svtkm::Id outputSize,
                                        Precision initValue = 1.f);

  ChannelBuffer<Precision> Copy();

  void InitConst(const Precision value);
  void InitChannels(const svtkm::cont::ArrayHandle<Precision>& signature);
  void Normalize(bool invert);
  void SetName(const std::string name);
  void Resize(const svtkm::Id newSize);
  void SetNumChannels(const svtkm::Int32 numChannels);
  svtkm::Int32 GetNumChannels() const;
  svtkm::Id GetSize() const;
  svtkm::Id GetBufferLength() const;
  std::string GetName() const;
  void AddBuffer(const ChannelBuffer<Precision>& other);
  void MultiplyBuffer(const ChannelBuffer<Precision>& other);
  /// Functions that are calleble within svtkm where the device is already known
  ///
  template <typename Device>
  SVTKM_CONT ChannelBuffer(const svtkm::Int32 size, const svtkm::Int32 numChannels, Device)
  {
    if (size < 1)
      throw svtkm::cont::ErrorBadValue("ChannelBuffer: Size must be greater that 0");
    if (numChannels < 1)
      throw svtkm::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that 0");

    this->NumChannels = numChannels;
    this->Size = size;

    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device());
  }



  template <typename Device>
  SVTKM_CONT void SetNumChannels(const svtkm::Int32 numChannels, Device)
  {
    if (numChannels < 1)
    {
      std::string msg = "ChannelBuffer SetNumChannels: numBins must be greater that 0";
      throw svtkm::cont::ErrorBadValue(msg);
    }
    if (this->NumChannels == numChannels)
      return;

    this->NumChannels = numChannels;
    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device());
  }

  template <typename Device>
  SVTKM_CONT void Resize(const svtkm::Id newSize, Device device)
  {
    if (newSize < 0)
      throw svtkm::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1 ");
    this->Size = newSize;
    this->Buffer.PrepareForOutput(this->Size * static_cast<svtkm::Id>(NumChannels), device);
  }
};
}
}
} // namespace svtkm::rendering::raytracing

#endif
