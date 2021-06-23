//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_encode2_h
#define svtk_m_worklet_zfp_encode2_h

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPEncode.h>
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
SVTKM_EXEC inline void GatherPartial2(Scalar* q,
                                     const PortalType& scalars,
                                     svtkm::Id offset,
                                     svtkm::Int32 nx,
                                     svtkm::Int32 ny,
                                     svtkm::Int32 sx,
                                     svtkm::Int32 sy)
{
  svtkm::Id x, y;
  for (y = 0; y < ny; y++, offset += sy - nx * sx)
  {
    for (x = 0; x < nx; x++, offset += 1)
      q[4 * y + x] = scalars.Get(offset);
    PadBlock(q + 4 * y, svtkm::UInt32(nx), 1);
  }
  for (x = 0; x < 4; x++)
    PadBlock(q + x, svtkm::UInt32(ny), 4);
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Gather2(Scalar* fblock,
                              const PortalType& scalars,
                              svtkm::Id offset,
                              int sx,
                              int sy)
{
  svtkm::Id counter = 0;

  for (svtkm::Id y = 0; y < 4; y++, offset += sy - 4 * sx)
    for (svtkm::Id x = 0; x < 4; x++, offset += sx)
    {
      fblock[counter] = scalars.Get(offset);
      counter++;
    }
}

struct Encode2 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id2 Dims;       // field dims
  svtkm::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  svtkm::Id2 ZFPDims;    // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block

public:
  Encode2(const svtkm::Id2 dims, const svtkm::Id2 paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    svtkm::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    svtkm::Id2 logicalStart = zfpBlock * svtkm::Id(4);
    svtkm::Id offset = logicalStart[1] * Dims[0] + logicalStart[0];

    constexpr svtkm::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];

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
      GatherPartial2(fblock, scalars, offset, nx, ny, 1, static_cast<svtkm::Int32>(Dims[0]));
    }
    else
    {
      Gather2(fblock, scalars, offset, 1, static_cast<svtkm::Int32>(Dims[0]));
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, svtkm::Int32(MaxBits), svtkm::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
