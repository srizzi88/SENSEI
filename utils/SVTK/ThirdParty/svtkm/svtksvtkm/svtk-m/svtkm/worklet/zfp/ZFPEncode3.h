//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_encode3_h
#define svtk_m_worklet_zfp_encode3_h

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
SVTKM_EXEC inline void GatherPartial3(Scalar* q,
                                     const PortalType& scalars,
                                     const svtkm::Id3 dims,
                                     svtkm::Id offset,
                                     svtkm::Int32 nx,
                                     svtkm::Int32 ny,
                                     svtkm::Int32 nz)
{
  svtkm::Id x, y, z;

  for (z = 0; z < nz; z++, offset += dims[0] * dims[1] - ny * dims[0])
  {
    for (y = 0; y < ny; y++, offset += dims[0] - nx)
    {
      for (x = 0; x < nx; x++, offset += 1)
      {
        q[16 * z + 4 * y + x] = scalars.Get(offset);
      }
      PadBlock(q + 16 * z + 4 * y, static_cast<svtkm::UInt32>(nx), 1);
    }

    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 16 * z + x, svtkm::UInt32(ny), 4);
    }
  }

  for (y = 0; y < 4; y++)
  {
    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 4 * y + x, svtkm::UInt32(nz), 16);
    }
  }
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Gather3(Scalar* fblock,
                              const PortalType& scalars,
                              const svtkm::Id3 dims,
                              svtkm::Id offset)
{
  // TODO: gather partial
  svtkm::Id counter = 0;
  for (svtkm::Id z = 0; z < 4; z++, offset += dims[0] * dims[1] - 4 * dims[0])
  {
    for (svtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
    {
      for (svtkm::Id x = 0; x < 4; x++, ++offset)
      {
        fblock[counter] = scalars.Get(offset);
        counter++;
      } // x
    }   // y
  }     // z
}

struct Encode3 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id3 Dims;       // field dims
  svtkm::Id3 PaddedDims; // dims padded to a multiple of zfp block size
  svtkm::Id3 ZFPDims;    // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block
public:
  Encode3(const svtkm::Id3 dims, const svtkm::Id3 paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
    ZFPDims[2] = PaddedDims[2] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr svtkm::Int32 BlockSize = 64;
    Scalar fblock[BlockSize];

    svtkm::Id3 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    zfpBlock[2] = blockIdx / (ZFPDims[0] * ZFPDims[1]);
    svtkm::Id3 logicalStart = zfpBlock * svtkm::Id(4);

    // get the offset into the field
    //svtkm::Id offset = (zfpBlock[2]*4*ZFPDims[1] + zfpBlock[1] * 4)*ZFPDims[0] * 4 + zfpBlock[0] * 4;
    svtkm::Id offset = (logicalStart[2] * Dims[1] + logicalStart[1]) * Dims[0] + logicalStart[0];

    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (logicalStart[2] + 4 > Dims[2])
      partial = true;
    if (partial)
    {
      const svtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? svtkm::Int32(Dims[0] - logicalStart[0]) : svtkm::Int32(4);
      const svtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? svtkm::Int32(Dims[1] - logicalStart[1]) : svtkm::Int32(4);
      const svtkm::Int32 nz =
        logicalStart[2] + 4 > Dims[2] ? svtkm::Int32(Dims[2] - logicalStart[2]) : svtkm::Int32(4);

      GatherPartial3(fblock, scalars, Dims, offset, nx, ny, nz);
    }
    else
    {
      Gather3(fblock, scalars, Dims, offset);
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;

    encoder.encode(fblock, svtkm::Int32(MaxBits), svtkm::UInt32(blockIdx), stream);
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
