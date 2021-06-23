//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_decode2_h
#define svtk_m_worklet_zfp_decode2_h

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
SVTKM_EXEC inline void ScatterPartial2(const Scalar* q,
                                      PortalType& scalars,
                                      const svtkm::Id2 dims,
                                      svtkm::Id offset,
                                      svtkm::Int32 nx,
                                      svtkm::Int32 ny)
{
  svtkm::Id x, y;
  for (y = 0; y < ny; y++, offset += dims[0] - nx, q += 4 - nx)
  {
    for (x = 0; x < nx; x++, offset++, q++)
    {
      scalars.Set(offset, *q);
    }
  }
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Scatter2(const Scalar* q,
                               PortalType& scalars,
                               const svtkm::Id2 dims,
                               svtkm::Id offset)
{
  for (svtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
  {
    for (svtkm::Id x = 0; x < 4; x++, ++offset)
    {
      scalars.Set(offset, *q++);
    } // x
  }   // y
}

struct Decode2 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id2 Dims;       // field dims
  svtkm::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  svtkm::Id2 ZFPDims;    // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block
public:
  Decode2(const svtkm::Id2 dims, const svtkm::Id2 paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr svtkm::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];
    // clear
    for (svtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, svtkm::Int32(MaxBits), static_cast<svtkm::UInt32>(blockIdx), stream);

    svtkm::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    svtkm::Id2 logicalStart = zfpBlock * svtkm::Id(4);

    svtkm::Id offset = logicalStart[0] + logicalStart[1] * Dims[0];
    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (partial)
    {
      const svtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? svtkm::Int32(Dims[0] - logicalStart[0]) : svtkm::Int32(4);
      const svtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? svtkm::Int32(Dims[1] - logicalStart[1]) : svtkm::Int32(4);
      ScatterPartial2(fblock, scalars, Dims, offset, nx, ny);
    }
    else
    {
      Scatter2(fblock, scalars, Dims, offset);
    }
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
