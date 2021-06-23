//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_decode1_h
#define svtk_m_worklet_zfp_decode1_h

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPDecode.h>
#include <svtkm/worklet/zfp/ZFPFunctions.h>
#include <svtkm/worklet/zfp/ZFPStructs.h>
#include <svtkm/worklet/zfp/ZFPTypeInfo.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void ScatterPartial1(const Scalar* q,
                                      PortalType& scalars,
                                      svtkm::Id offset,
                                      svtkm::Int32 nx)
{
  svtkm::Id x;
  for (x = 0; x < nx; x++, offset++, q++)
  {
    scalars.Set(offset, *q);
  }
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Scatter1(const Scalar* q, PortalType& scalars, svtkm::Id offset)
{
  for (svtkm::Id x = 0; x < 4; x++, ++offset)
  {
    scalars.Set(offset, *q++);
  } // x
}

struct Decode1 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id Dims;        // field dims
  svtkm::Id PaddedDims;  // dims padded to a multiple of zfp block size
  svtkm::Id ZFPDims;     // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block
public:
  Decode1(const svtkm::Id dims, const svtkm::Id paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr svtkm::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];
    // clear
    for (svtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, svtkm::Int32(MaxBits), static_cast<svtkm::UInt32>(blockIdx), stream);


    svtkm::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    svtkm::Id logicalStart = zfpBlock * svtkm::Id(4);

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;
    if (partial)
    {
      const svtkm::Int32 nx =
        logicalStart + 4 > Dims ? svtkm::Int32(Dims - logicalStart) : svtkm::Int32(4);
      ScatterPartial1(fblock, scalars, logicalStart, nx);
    }
    else
    {
      Scatter1(fblock, scalars, logicalStart);
    }
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
